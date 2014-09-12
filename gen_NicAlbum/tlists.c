#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "gen.h"
#include "resource.h"

#include "wa_ipc.h"
#include "ipc_pe.h"
#include "tlists.h"

#include "ashuffle.h"

extern winampGeneralPurposePlugin plugin;

extern char debug_string[1024];

struct t_type_s
{
    size_t size;
    unsigned char* type;
};

struct t_type_s track_type = {0, NULL};

struct tracks_s
{
    size_t cap;
    size_t size;
    int* items;
};

struct tracks_s tracks_master = {0, 0, NULL};

struct tracks_s tracks = {0, 0, NULL};

// Separators
const char sep_album[] = "-----";
const char sep_random[] = "=====";

int merror = 0;

#define MIN_SEP_LEN 5

/*
 * Build the album list:
 * List of all playable items:
 * All random tracks (start of playlist and after ===== separators)
 * The first track of each album (after a ----- separator)
 *
 * Also builds a list of the type of each track
 */
void build_master()
{
    int mode = 0;
	int error = 0;
    
    // Playable items
    int j = 0;
    
    int i;
    
    // Keep track of first track in album
    int first_atrack = 0;
    
    HWND playlist_wnd = get_playlist_hwnd();
    
    // Get playlist length
    int pl_len = SendMessage( plugin.hwndParent, WM_WA_IPC, 0, IPC_GETLISTLENGTH );
    
    // Get the next power of 2 for malloc purposes
    size_t pl_length_2 = next_pow2( pl_len );
    
    if ( pl_len <= 0 )
    {
		OutputDebugString( "Build master: Playlist empty" );
        return;
    }
    
    // Doesn't exist yet, malloc
    if ( track_type.type == NULL )
    {
        track_type.type = malloc( pl_length_2 * sizeof( unsigned char ) );
        
        if ( track_type.type == NULL )
        {
            OutputDebugString( "Build master: Couldn't alloc track_type" );
            
			error = 1;
        }
    }
    // Exists, if we need to resize
    else
    {
        // Need to resize
        if ( pl_length_2 > tracks_master.cap || pl_length_2 <= tracks_master.cap / 4 )
        {
			unsigned char* temp;

            wsprintf( debug_string, "Build master: Resizing lists to size %d:", pl_length_2 );
            OutputDebugString( debug_string );            
            
			temp = realloc( track_type.type, pl_length_2 * sizeof( unsigned char ) );
            
            if ( temp == NULL )
            {
                OutputDebugString( "Build master: Couldn't realloc track_type" );
                
				error = 1;
            }
			else
			{
				track_type.type = temp;
			}
        }
        else
        {
            OutputDebugString( "Build master: Not resizing master track list" );
        }
    }

	// Set size of type list
	track_type.size = pl_len;

	    // Doesn't exist yet, malloc
    if ( track_type.type == NULL )
    {
        tracks_master.items = malloc( pl_length_2 * sizeof( int ) );
        
        if ( tracks_master.items == NULL )
        {
            OutputDebugString( "Build master: Couldn't alloc tracks" );
            
			error = 1;
        }
    }
    // Exists, if we need to resize
    else
    {
        // Need to resize
        if ( pl_length_2 > tracks_master.cap || pl_length_2 <= tracks_master.cap / 4 )
        {
            int* temp = realloc( tracks_master.items, pl_length_2 * sizeof( int ) );
            
            if ( temp == NULL )
            {
                OutputDebugString( "Build master: Couldn't realloc tracks_master" );
                error = 1;
            }
			else
			{
				tracks_master.items = temp;
			}
        }
    }
    
    // Set the capacity
    tracks_master.cap = pl_length_2;

	merror = error;

	if(!error)
	{
		for ( i = 0; i < pl_len; i++ )
		{
			int ret;
	        
			fileinfo2 file;
	        
			// Get the file name
			file.fileindex = i; // this is zero based!
			ret = SendMessage( playlist_wnd, WM_WA_IPC, IPC_PE_GETINDEXTITLE, ( LPARAM ) & file );
	        
			// If it returns 0 then track information was received
			if ( !ret )
			{
				// Random mode
				if ( mode == 0 )
				{
					// Note the playlist item type at every step
	                
					// If we see an album separator, switch to album mode
					if ( strncmp( file.filetitle, sep_album, MIN_SEP_LEN ) == 0 )
					{
						track_type.type[i] = IS_ALBUM_SEP;
	                    
						mode = 1;
	                    
						first_atrack = 1;
					}
					// If we see a random separator
					else if ( strncmp( file.filetitle, sep_random, MIN_SEP_LEN ) == 0 )
					{
						track_type.type[i] = IS_RANDOM_SEP;
					}
					// Normal random track, add to albums / tracks
					else
					{
						track_type.type[i] = IS_RANDOM;
						tracks_master.items[j++] = i;
					}
				}
				// Album mode
				else
				{
					// Note the playlist item type at every step
	                
					// If we see an album, note it
					if ( strncmp( file.filetitle, sep_album, MIN_SEP_LEN ) == 0 )
					{
						track_type.type[i] = IS_ALBUM_SEP;
	                    
						// Note the last album song
						if ( i > 0 && track_type.type[i - 1] == IS_ALBUM )
						{
							track_type.type[i - 1] = IS_ALBUM_LAST;
						}
	                    
						first_atrack = 1;
					}
					// If we see a random separator, switch mode to random mode
					else if ( strncmp( file.filetitle, sep_random, MIN_SEP_LEN ) == 0 )
					{
						track_type.type[i] = IS_RANDOM_SEP;
	                    
						// Note the last album song
						if ( i > 0 && track_type.type[i - 1] == IS_ALBUM )
						{
							track_type.type[i - 1] = IS_ALBUM_LAST;
						}
	                    
						mode = 0;
					}
					// Normal random track. If first track in the album, add it to albums / tracks
					else
					{
						track_type.type[i] = IS_ALBUM;
	                    
						if ( first_atrack )
						{
							tracks_master.items[j++] = i;
	                        
							first_atrack = 0;
						}
					}
				}
			}
		}
	}

    // Record size of the albums / tracks list
    tracks_master.size = j;
}

/*
 * Copy the track list from the master when we need to regenerate the list
 * of random tracks. No need to rebuild if the playlist hasn't changed.
 */
void copy_master()
{
    // Get the next power of 2 from the master list
    size_t atracks_m_size2 = next_pow2( tracks_master.size );

	if ( merror )
	{
		OutputDebugString( "Copy master: In memory error state" );
		return;
	}

	if ( tracks_master.size <= 0 )
    {
		OutputDebugString( "Copy master: No playable tracks" );
        
		atracks_m_size2 = 1;
    }
    
    // Doesn't exist yet, malloc
    if ( tracks.items == NULL )
    {
        tracks.items = malloc( atracks_m_size2 * sizeof( int ) );
        
        if ( tracks.items == NULL )
        {
            OutputDebugString( "Copy master: Couldn't alloc tracks" );
            
			merror = 1;
        }
    }
    // Exists, check for resize
    else
    {
        // Need to resize
        if ( atracks_m_size2 > tracks.cap || atracks_m_size2 <= tracks.cap / 4 )
        {
            int* temp = realloc( tracks.items, atracks_m_size2 * sizeof( int ) );
            
            if ( temp == NULL )
            {
                OutputDebugString( "Copy master: Couldn't realloc tracks" );
                
				merror = 1;
            }
			else
			{
				tracks.items = temp;
			}
        }
        else
        {
            OutputDebugString( "Copy master: Not resizing track lists" );
        }
    }
    
    // Set the capacity
    tracks.cap = atracks_m_size2;
    
    // Copy the size
    tracks.size = tracks_master.size;
    
    // Copy the tracks over if no errors occured
	if(!merror)
	{
		memcpy( tracks.items, tracks_master.items, tracks.size * sizeof( int ) );
	}
}

// Get the status of an item
unsigned char atrack_type( int i )
{
    // Sanity
    if( track_type.type == NULL || merror )
	{
		OutputDebugString( "Atrack Type: In memory error state" );
		return IS_RANDOM;
	}
    
    // In the bounds
	if( i < 0 || (size_t) i >= track_type.size )
	{
		OutputDebugString( "Atrack Type: Invalid index" );
		return IS_RANDOM;
	}
    
    return track_type.type[i];
}

// Get the size of the albums / tracks remaining
unsigned int atrack_size()
{
	if( merror )
	{
		OutputDebugString( "Atrack Size: In memory error state" );
		return 0;
	}

    return tracks.size;
}

// Remove an album / track
int rm_atrack( unsigned int i )
{
    int track;
    
    if( merror )
	{
		OutputDebugString( "Remove Atrack: In memory error state" );
		return -1;
	}
	
	// Non empty
    if ( tracks.size == 0 )
    {
        OutputDebugString( "Albums / tracks list empty" );
        return -1;
    }
    
    // Valid track
    if ( i >= tracks.size )
    {
        OutputDebugString( "Invalid index to remove from albums / tracks" );
        return -1;
    }
    
    track = tracks.items[i];
    
    // Replace it with the one at the end
    tracks.items[i] = tracks.items[tracks.size - 1];
    
    tracks.size--;
    
    return track;
}

// Utility functions

// Get the next power of 2. 32 bit only.
size_t next_pow2( int value )
{
    value--;
    value = ( value >> 1 ) | value;
    value = ( value >> 2 ) | value;
    value = ( value >> 4 ) | value;
    value = ( value >> 8 ) | value;
    value = ( value >> 16 ) | value;
    // value = (value >> 32) | value; // For 64 bit
    
    value++; // Val is now the next highest power of 2.
    
    return ( size_t ) value;
}

// Frees all the lists.
void free_lists()
{
	if( track_type.type != NULL )
		free( track_type.type );

	if( tracks_master.items != NULL )
		free( tracks_master.items );
    
	if( tracks.items != NULL )
		free( tracks.items );
    
    track_type.type = NULL;
    tracks_master.items = NULL;
    tracks.items = NULL;
}