#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "disk.h"

extern ssize_t pread (int __fd, void *__buf, size_t __nbytes, __off_t __offset);
extern ssize_t pwrite (int __fd, const void *__buf, size_t __nbytes, __off_t __offset);

struct disk {
	int fd;
	int block_size;
	int nblocks;
};


/* disk_open()
 * Create a new virtual disk in the file "filename", with the given number of blocks.
 * Returns a pointer to a new disk object, or null on failure.
 */
struct disk * disk_open(const char *diskname, int nblocks )
{
	struct disk *d;

	// allocate space for disk information structure
	d = malloc(sizeof(*d));
	if(!d) return 0;

	// open a file to serve as the virtual disk
	d->fd = open(diskname, O_CREAT|O_RDWR, 0777);
	if(d->fd<0) {
		free(d);
		return 0;
	}

	// set block size and hwo large the disk will be
	d->block_size = BLOCK_SIZE;
	d->nblocks = nblocks;

	// make file the right size with '\0'
	if(ftruncate(d->fd, d->nblocks*d->block_size)<0) {
		close(d->fd);
		free(d);
		return 0;
	}

	return d;
}

/* disk_write()
 * Write exactly BLOCK_SIZE bytes to a given block on the virtual disk.
 * "d" must be a pointer to a virtual disk, "block" is the block number,
 * 	and "data" is a pointer to the data to write.
 */
void disk_write(struct disk *d, int block, const char *data )
{
	// is this a valid block number?
	if(block<0 || block >= d->nblocks) {
		fprintf(stderr, "disk_write: invalid block #%d\n",block);
		abort();
	}

	// write one block of 'data' to disk file at offset (block # * block size)
	int actual = pwrite(d->fd, data, d->block_size, block*d->block_size);
	if(actual != d->block_size) {
		fprintf(stderr, "disk_write: failed to write block #%d: %s\n",block,strerror(errno));
		abort();
	}
}

/* disk_read()
 * Read exactly BLOCK_SIZE bytes from a given block on the virtual disk.
 * "d" must be a pointer to a virtual disk, "block" is the block number,
 * 	and "data" is a pointer to where the data will be placed.
 */
void disk_read(struct disk *d, int block, char *data )
{
	// is this a valid block number?
	if(block<0 || block >= d->nblocks) {
		fprintf(stderr, "disk_read: invalid block #%d\n", block);
		abort();
	}

	// read one block into 'data'
	int actual = pread(d->fd, data, d->block_size, block*d->block_size);
	if(actual != d->block_size) {
		fprintf(stderr, "disk_read: failed to read block #%d: %s\n", block, strerror(errno));
		abort();
	}
}

/* disk_nblocks()
 * Return the number of blocks in the virtual disk.
 */
int disk_nblocks(struct disk *d )
{
	// basically a get function
	return d->nblocks;
}

/* disk_close()
 * Close the virtual disk.
 */
void disk_close(struct disk *d )
{
	close(d->fd);
	free(d);
}
