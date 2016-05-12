#ifndef DISK_H
#define DISK_H

#define BLOCK_SIZE 4096

struct disk * disk_open( const char *filename, int blocks );
void disk_write( struct disk *d, int block, const char *data );
void disk_read( struct disk *d, int block, char *data );
int disk_nblocks( struct disk *d );
void disk_close( struct disk *d );

#endif
