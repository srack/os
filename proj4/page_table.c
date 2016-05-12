#define _GNU_SOURCE

#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <limits.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ucontext.h>

#include "page_table.h"

struct page_table {
	int fd;
	char *virtmem;
	int npages;
	char *physmem;
	int nframes;
	int *page_mapping;
	int *page_bits;
	page_fault_handler_t handler;
};

// used to globally keep a copy of the page table last used with one of the functions
struct page_table *the_page_table = 0;

static void internal_fault_handler( int signum, siginfo_t *info, void *context )
{
	#ifdef i386
	char *addr = (char*)(((struct ucontext *)context)->uc_mcontext.cr2);
	#else
	char *addr = info->si_addr;
	#endif

	// if the page table has been created already, then we will handle the fault
	struct page_table *pt = the_page_table;
	if(pt) {
		int page = (addr - pt->virtmem) / PAGE_SIZE;

		// if it is a valid page number, call the handler
		if(page >= 0 && page < pt->npages) {
			pt->handler(pt, page);
			return;
		}
	}

	fprintf(stderr, "segmentation fault at address %p\n", addr);
	abort();
}


/* page_table_create()
 * Create a new page table, along with a corresponding virtual memory
 * 	that is "npages" big and a physical memory that is "nframes" big
 * When a page fault occurs, the routine pointed to by "handler" will be called. 
 */
struct page_table *page_table_create( int npages, int nframes, page_fault_handler_t handler )
{
	int i;
	struct sigaction sa;
	struct page_table *pt;
	char filename[256];

	// allocate space for the page table information structure
	pt = malloc(sizeof(struct page_table));
	if(!pt) return 0;

	// set the global page table to this also so we will handle page faults
	the_page_table = pt;

	// the file will be in /tmp and specified by the process id and the domain identifier
	sprintf(filename,"/tmp/pmem.%d.%d",getpid(),getuid());

	// open this file, which will contain ... ****** 
	pt->fd = open(filename,O_CREAT|O_TRUNC|O_RDWR,0777);
	if(!pt->fd) return 0;

	// make it the right size for memory = number of pages (so virtual memory)
	ftruncate(pt->fd, PAGE_SIZE*npages);

	// deletes the file name from the file system, and after it is closed by this program, will
	//	delete the file as well
	unlink(filename);

	// pt->physmem is starting address of mapping for physical memory
	pt->physmem = mmap(0, nframes*PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, pt->fd, 0);
	pt->nframes = nframes;

	// pt->virtmem is starting address of mapping for virtual memory
	pt->virtmem = mmap(0, npages*PAGE_SIZE, PROT_NONE, MAP_SHARED|MAP_NORESERVE, pt->fd, 0);
	pt->npages = npages;

	// in page table, need bits to specify RWX, 0 when not valid (not in mem)
	pt->page_bits = malloc(sizeof(int)*npages);
	// integers for frame number of the page in memory
	pt->page_mapping = malloc(sizeof(int)*npages);

	// handler for a page fault
	pt->handler = handler;

	for (i = 0; i < pt->npages; i++) pt->page_bits[i] = 0;

	sa.sa_sigaction = internal_fault_handler;
	sa.sa_flags = SA_SIGINFO;

	sigfillset( &sa.sa_mask );
	sigaction( SIGSEGV, &sa, 0 );

	// give the program back the initialized page table
	return pt;
}


/* page_table_delete()
 * Delete a page table and the corresponding virtual and physical memories.
 */
void page_table_delete( struct page_table *pt )
{
	// undo the memory map for physical and virtual memory
	munmap(pt->virtmem, pt->npages * PAGE_SIZE);
	munmap(pt->physmem, pt->nframes * PAGE_SIZE);
	// free malloc-ed stuff and close the file (deletes it also)
	free(pt->page_bits);
	free(pt->page_mapping);
	close(pt->fd);
	free(pt);
}


/* page_table_set_entry()
 * Set the frame number and access bits associated with a page.
 * The bits may be any of PROT_READ, PROT_WRITE, or PROT_EXEC logical-ored together.
 */
void page_table_set_entry( struct page_table *pt, int page, int frame, int bits )
{
	// is this a valid page number?
	if (page < 0 || page >= pt->npages) {
		fprintf(stderr, "page_table_set_entry: illegal page #%d\n", page);
		abort();
	}
	// is this a valid frame number?
	if (frame < 0 || frame >= pt->nframes) {
		fprintf(stderr, "page_table_set_entry: illegal frame #%d\n", frame);
		abort();
	}

	// change the page -> frame mapping specified
	pt->page_mapping[page] = frame;
	pt->page_bits[page] = bits;

	remap_file_pages(pt->virtmem + page * PAGE_SIZE, PAGE_SIZE, 0, frame, 0);
	mprotect(pt->virtmem + page * PAGE_SIZE, PAGE_SIZE, bits);
}

/* page_table_get_entry()
 * Get the frame number and access bits associated with a page.
 * "frame" and "bits" must be pointers to integers which will be filled with the current values.
 * The bits may be any of PROT_READ, PROT_WRITE, or PROT_EXEC logical-ored together.
 */
void page_table_get_entry( struct page_table *pt, int page, int *frame, int *bits )
{
	// is it a valid page table entry?
	if( page < 0 || page >= pt->npages ) {
		fprintf(stderr,"page_table_get_entry: illegal page #%d\n",page);
		abort();
	}

	*frame = pt->page_mapping[page];
	*bits = pt->page_bits[page];
}

/* page_table_print_entry()
 * Print out the page table entry for a single page.
 */
void page_table_print_entry( struct page_table *pt, int page )
{
	if( page<0 || page>=pt->npages ) {
		fprintf(stderr,"page_table_print_entry: illegal page #%d\n",page);
		abort();
	}

	int b = pt->page_bits[page];

	printf("page %06d: frame %06d bits %c%c%c\n",
		page,
		pt->page_mapping[page],
		b&PROT_READ  ? 'r' : '-',
		b&PROT_WRITE ? 'w' : '-',
		b&PROT_EXEC  ? 'x' : '-'
	);

}

/* page_table_print()
 * Print out the state of every page in a page table.
 */
void page_table_print( struct page_table *pt )
{
	int i;
	for(i=0;i<pt->npages;i++) {
		page_table_print_entry(pt,i);
	}
}

/* page_table_get_nframes()
 * Return the total number of frames in the physical memory.
 */
int page_table_get_nframes( struct page_table *pt )
{
	return pt->nframes;
}

/* page_table_get_npages()
 * Return the total number of pages in the virtual memory.
 */
int page_table_get_npages( struct page_table *pt )
{
	return pt->npages;
}

/* page_table_get_virtmem()
 * Return a pointer to the start of the virtual memory associated with a page table. 
 */
char *page_table_get_virtmem( struct page_table *pt )
{
	return pt->virtmem;
}

/* page_table_get_pysmem()
 *  Return a pointer to the start of the physical memory associated with a page table.
 */
char *page_table_get_physmem( struct page_table *pt )
{
	return pt->physmem;
}
