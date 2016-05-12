/* Sam Rack
 * CSE 30341 - Operating Systems
 * Project 4 - Virtual Memory
 * main.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "page_table.h"
#include "disk.h"
#include "program.h"

#define MAX_LRU 0 

//////////////////////
// GLOBAL VARIABLES //
//////////////////////
char *alg = NULL;
int *reverse_pt = NULL;
struct disk *disk = NULL;
int pageFaults = 0;
int diskReads = 0;
int diskWrites = 0;
int *lruData = NULL;
int lruUpdate = 0;

/////////////////////////
// FUNCTION PROTOTYPES //
/////////////////////////
void page_fault_handler(struct page_table *pt, int page);
int pf_RANDOM(struct page_table *pt);
int pf_FIFO(struct page_table *pt);
int pf_CUSTOM(struct page_table *pt);


////////////
// main() //
////////////
int main( int argc, char *argv[] )
{
	// check arg count
	if(argc!=5) {
		printf("usage: virtmem <npages> <nframes> <rand|fifo|custom> <sort|scan|focus>\n");
		return 1;
	}

	// get the arguments
	int npages = atoi(argv[1]);
	int nframes = atoi(argv[2]);
	if (npages <= 0 || nframes <= 0) {
		printf("npages and nframes must be larger than 0.\n");
		return 1;
	}

	alg = argv[3];
	if (!strcmp(alg, "rand") && !strcmp(alg, "fifo") && !strcmp(alg, "custom")) {
		printf("unknown page replacement algorithm: %s\n", alg);
		return 1;
	}
	const char *program = argv[4];

	// create the virtual disk
	disk = disk_open("myvirtualdisk", npages);
	if(!disk) {
		fprintf(stderr, "couldn't create virtual disk: %s\n", strerror(errno));
		return 1;
	}

	// create the page table
	struct page_table *pt = page_table_create(npages, nframes, page_fault_handler);
	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
		return 1;
	}

	// create the reverse page table - indexed by frame number so don't have to search
	// 	page table for what page goes with a particular frame, also keeps track
	// 	of free frames (= -1) in this case
	reverse_pt = malloc(page_table_get_nframes(pt) * sizeof(int));
	if (!reverse_pt) {
		fprintf(stderr, "couldn't create reverse page table: %s\n", strerror(errno));
		return 1;
	}
	int i; 
	for (i = 0; i < page_table_get_nframes(pt); ++i) reverse_pt[i] = -1;

	// initialize the LRU bits to 0
	lruData = malloc(page_table_get_nframes(pt) * sizeof(int));
	if (!lruData) {
		fprintf(stderr, "couldn't create lru data: %s\n", strerror(errno));
		return 1;
	}
	for (i = 0; i < page_table_get_nframes(pt); ++i) lruData[i] = 0;

	char *virtmem = page_table_get_virtmem(pt);

	// run the specified program
	if(!strcmp(program,"sort")) sort_program(virtmem, npages*PAGE_SIZE);
	else if(!strcmp(program,"scan")) scan_program(virtmem, npages*PAGE_SIZE);
	else if(!strcmp(program,"focus")) focus_program(virtmem, npages*PAGE_SIZE);
	else fprintf(stderr, "unknown program: %s\n", argv[3]);

	printf("page faults: %d\ndisk reads: %d\ndisk writes: %d\n", pageFaults, diskReads, diskWrites);

	// clean up
	page_table_delete(pt);
	disk_close(disk);
	free(reverse_pt);
	free(lruData);

	return 0;
}

//////////////////////////
// page_fault_handler() //
//////////////////////////
void page_fault_handler(struct page_table *pt, int page) {
	/** (1) check if page fault happened because page is being written to for the first time **/
	int frame, bits;
	page_table_get_entry(pt, page, &frame, &bits);

	// if the read bit is set, then it is already in memory and the write bit just has to added
	if (bits & PROT_READ) {	// will be non-zero if the read bit is set
		// or the original bits with PROT_WRITE to add that permission
		bits = bits | PROT_WRITE;
		// use the original page/frame mapping so only the bits change
		page_table_set_entry(pt, page, frame, bits);

		// update the lru data for that page
		lruData[frame] = MAX_LRU;

		return;
	}

	// page faults should only be counted if they aren't because of adding a write bit
	++pageFaults;

	// increment lru update counter
	++lruUpdate;
	if (lruUpdate % 5 == 0) { 
		int i;
		for (i = 0; i < page_table_get_nframes(pt); ++i) {
			--lruData[i];
		}
	}

	/** (2) check if there is a free frame to which the page could be mapped **/
	for (frame = 0; frame < page_table_get_nframes(pt); ++frame) {
		if (reverse_pt[frame] == -1) break;
	}

	if (frame < page_table_get_nframes(pt)) {
		// pull it into memory at location frame -- disk read into memory
		// the block location corresponds to the start of the page location in virtual mem
		char *frameStart = page_table_get_physmem(pt) + frame*PAGE_SIZE;
		disk_read(disk, page, frameStart);
		++diskReads;

		// create the pt entry with the new page-frame mapping and PROT_READ bit set
		page_table_set_entry(pt, page, frame, PROT_READ);
		
		// update the lru data for that page
		lruData[frame] = MAX_LRU;

		// change reverse_pt[frame] to reflect that it now has page's data in it
		reverse_pt[frame] = page;

		return;	
	}


	/** (3) if there are no free frames, then call the specified algorithm for page replacement **/
	if (!strcmp(alg, "rand")) frame = pf_RANDOM(pt);		
	else if (!strcmp(alg, "fifo")) frame = pf_FIFO(pt);
	else if (!strcmp(alg, "custom")) frame = pf_CUSTOM(pt);
	else {
		// this case should never happen because a check is done earlier
		printf("algorithm type not recognized: %s\n", alg);
		exit(1);
	}
	
	// find what was chosen as victim
	int victPage = reverse_pt[frame];
	int victBits;
	page_table_get_entry(pt, victPage, &frame, &victBits);

	// check if the victim page is dirty and has to be written back
	if (victBits & PROT_WRITE) {
		char *frameStart = page_table_get_physmem(pt) + frame*PAGE_SIZE;
		disk_write(disk, victPage, frameStart);
		++diskWrites;
	}

	// then, read from disk whatever page we need
	char *frameStart = page_table_get_physmem(pt) + frame*PAGE_SIZE;
	disk_read(disk, page, frameStart);
	++diskReads;

	// update the page table
	page_table_set_entry(pt, victPage, frame, 0);
	page_table_set_entry(pt, page, frame, PROT_READ);

	// update the lru data for that page
	lruData[frame] = MAX_LRU;	
	
	// update the reverse page table
	reverse_pt[frame] = page;

}

/////////////////
// pf_RANDOM() //
/////////////////
int pf_RANDOM(struct page_table *pt) {
	int victim = lrand48() % page_table_get_nframes(pt);	
	return victim;
}


///////////////
// pf_FIFO() //
///////////////
int pf_FIFO(struct page_table *pt) {
	static int index = 0;

	// when the pages are initially filled in, it is in order (ie. first page in is mapped 
	// 	to frame 0, second page to frame 1, ...)
	// so simply increasing the index from 0 will be FIFO behavior

	int victim = index;

	// update the index for the next one
	index = (index + 1) % page_table_get_nframes(pt);
	return victim;
}


/////////////////
// pf_CUSTOM() //
/////////////////
int pf_CUSTOM(struct page_table *pt) {

	// this is an LRU algorithm, but it also takes into account whether or not the data has to be written back

	int minWrite = MAX_LRU + 1;
	int minNoWrite = MAX_LRU + 1;
	int victimW = -1;
	int victimNW = -1;
	int frame, bits;
	int i;
	for (i = 0; i < page_table_get_nframes(pt); ++i) {
		page_table_get_entry(pt, reverse_pt[i], &frame, &bits);

		if (!(bits & PROT_WRITE)) {
			if (lruData[i] < minNoWrite) {
				victimNW = i;
				minNoWrite = lruData[i];
			}
		} else if (lruData[i] < minWrite) {
			victimW = i;
			minWrite = lruData[i];
		}


	} 

	int diff = minNoWrite - minWrite;
	int thresh = page_table_get_nframes(pt)/2;

	if (victimNW == -1) return victimW;
	if (victimW == -1) return victimNW; 

	if (diff < thresh) return victimNW;
	return victimW;

}














