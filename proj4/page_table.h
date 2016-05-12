#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H

#include <sys/mman.h>

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

struct page_table;

/* typedef for the pointer to the page fault handler function that will take the
 	page table the fault happened for and the page number as arguments */
typedef void (*page_fault_handler_t) (struct page_table *pt, int page);

struct page_table *page_table_create( int npages, int nframes, page_fault_handler_t handler );
void page_table_delete( struct page_table *pt );
void page_table_set_entry( struct page_table *pt, int page, int frame, int bits );
void page_table_get_entry( struct page_table *pt, int page, int *frame, int *bits );
char *page_table_get_virtmem( struct page_table *pt );
char *page_table_get_physmem( struct page_table *pt );
int page_table_get_nframes( struct page_table *pt );
int page_table_get_npages( struct page_table *pt );
void page_table_print_entry( struct page_table *pt, int page );
void page_table_print( struct page_table *pt );

#endif
