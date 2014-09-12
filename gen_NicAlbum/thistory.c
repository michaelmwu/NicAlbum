#include <assert.h>
#include <windows.h>
#include "thistory.h"

extern char debug_string[1024];

// History
t_history history = {0, -1};

int history_size()
{
    return history.size;
}

int history_pop()
{
    int track;
    
    assert( history.size > 0 );
    
    history.size--;
    
    track = history.tracks[history.cur];
    
    history.cur = ( history.cur - 1 ) % HISTORY_SIZE;

	if(history.cur < 0)
		history.cur = HISTORY_SIZE - 1;
    
    wsprintf( debug_string, "History: popping track: %d", track );
    OutputDebugString( debug_string );

	wsprintf( debug_string, "Current size: %d", history.size );
    OutputDebugString( debug_string );

    return track;
}

// Add to history, overwriting bottom of stack if stack is full
void history_add( int index )
{
    if ( history.size < HISTORY_SIZE )
        history.size++;
    
    history.cur = ( history.cur + 1 ) % HISTORY_SIZE;
    
    history.tracks[history.cur] = index;
    
    wsprintf( debug_string, "History: adding track: %d", index );
    OutputDebugString( debug_string );

	wsprintf( debug_string, "History size: %d", history.size );
    OutputDebugString( debug_string );
}

void history_clear()
{
    history.size = 0;
}