// Prevent double inclusion
#ifndef TLISTS_H_
#define TLISTS_H_

#define IS_RANDOM		0
#define IS_RANDOM_SEP	1
#define IS_ALBUM		2
#define IS_ALBUM_LAST	3
#define IS_ALBUM_SEP	4

// Build the master list
void build_master();

// Copy the master list to the working list
void copy_master();

// Get the status of a type
unsigned char atrack_type( int i );

// Get the size of the albums / tracks remaining
unsigned int atrack_size();

// Remove an album / track
int rm_atrack( unsigned int i );

size_t next_pow2( int value );

// Free the lists
void free_lists();

#endif