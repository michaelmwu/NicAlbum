// Winamp general purpose plug-in mini-SDK
// Copyright (C) 1997, Justin Frankel/Nullsoft

#include <windows.h>
#include <stdlib.h>

#include "gen.h"
#include "resource.h"

#include "wa_ipc.h"
#include "ipc_pe.h"
#include <time.h>
#include "mt19937ar.h"

#include "ashuffle.h"
#include "thistory.h"
#include "tlists.h"

winampGeneralPurposePlugin plugin =
{
    GPPHDR_VER,
    PLUGIN_DESC " v1.3",
    init,
    config,
    quit,
};

WNDPROC lpWndProcOld;
HINSTANCE h_Library = NULL;

int index;
int ft_index;

int pl_len;

HWND playlist_wnd = 0;
fileinfo2 file;
int wa_version;
int ret = 0;

int next;

int master_built = 0;
int is_shuffle;
int is_repeat;

int status;
int is_pause;

int cfg_enabled = 0;
const char ini_file[] = "gen_nic.ini";

char debug_string[1024];


BOOL CALLBACK ConfigProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK WndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );

// avoid CRT. Evil. Big. Bloated.
BOOL WINAPI _DllMainCRTStartup( HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved )
{
    return TRUE;
}

// To avoid some errors when closing Winamp, see winamp forums
BOOL WINAPI DllMain( HANDLE hInstance, DWORD dwReason, LPVOID pvUnused )
{

    switch ( dwReason )
    {
        case DLL_PROCESS_ATTACH:
            break;
            
        case DLL_PROCESS_DETACH:
            if ( h_Library )
            {
                FreeLibrary( h_Library );
                h_Library = NULL;
            }
            break;
    }
    
    return TRUE;
}

// Called when the user opens plugin's configuration
void config()
{
    DialogBox( plugin.hDllInstance, MAKEINTRESOURCE( IDD_DIALOG1 ), plugin.hwndParent, ConfigProc );
}

// init
int init()
{
    char filename[512];
    
    config_read();
    
    init_genrand(( unsigned )time( NULL ) );
    
    // get the name of the currently executing DLL (in case someone renames the .dll file)
    GetModuleFileName( plugin.hDllInstance, filename, sizeof( filename ) );
    h_Library = LoadLibrary( filename );
    
    lpWndProcOld = ( WNDPROC )SetWindowLong( plugin.hwndParent, GWL_WNDPROC, ( LONG )WndProc );
    
    master_built = 1;
    build_master();
    copy_master();
    
    return 0;
}

//quit
void quit()
{
    free_lists();
    
    config_write();
    SetWindowLong( plugin.hwndParent, GWL_WNDPROC, ( LONG )lpWndProcOld );
}


LRESULT CALLBACK WndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    // Build list when first playing a file
    if ( uMsg == WM_USER && lParam == IPC_PLAYING_FILE )
    {
        OutputDebugString( "IPC_PLAYING_FILE" );
        OutputDebugString(( char* )wParam );
        
        if ( !master_built )
        {
			OutputDebugString( "Play: Rebuilding track lists" );
            
            build_master();
            copy_master();
            
            master_built = 1;
        }
        
        // Restore playing status
        if ( is_pause )
        {
            OutputDebugString( "Pausing:" );
            SendMessage( hwnd, WM_COMMAND, WINAMP_PAUSE, 0 );
        }
        
        is_pause = 0;
    }
    
    // Mark the playlist as dirty upon any modification
    if ( uMsg == WM_USER && lParam == IPC_PLAYLIST_MODIFIED )
    {
        master_built = 0;
        
        // Clear the history
        history_clear();
    }
    
    
    if ( uMsg == WM_USER && lParam == IPC_GET_PREVIOUS_PLITEM )
    {
        // If plugin not enabled, let winamp do default action
        if ( !cfg_enabled )
            return -1;
            
        // If there is history, return that and rebuild the random list
        if ( history_size() > 0 )
        {
			// Get playing status
            status = SendMessage( hwnd, WM_WA_IPC, 0, IPC_ISPLAYING );

            // Pretend it is paused if anything but playing.
            is_pause = ( status != IS_PLAY );
            
            if ( is_pause )
                OutputDebugString( "Prev track: Is paused" );

			OutputDebugString( "Prev track: Rebuilding track lists" );

            copy_master();
            return history_pop();
        }
    }
    
    if ( uMsg == WM_USER && lParam == IPC_GET_NEXT_PLITEM )
    {
        // Playlist is dirty! Rebuild
        if ( !master_built )
        {
			OutputDebugString( "Next track: Rebuilding track lists" );
            
            build_master();
            copy_master();
            
            master_built = 1;
        }

		if(	atrack_size() == 0 )
		{
			/* Apparently Winamp crashes anyways if your playlist is all separators
			 * and you can't do anything to stop it. This is stupid. */

			OutputDebugString( "Next track: No playable tracks" );
			return -1;
		}

        index = SendMessage( plugin.hwndParent, WM_WA_IPC, 0, IPC_GETLISTPOS );
        
        // Check for any other plugin that wants to modify the next item
        // Especially JTFE
        ft_index = CallWindowProc( lpWndProcOld, hwnd, uMsg, wParam, lParam );
        
        if ( ft_index != -1 )
        {
            wsprintf( debug_string, "Next track: Fall through index: %d", ft_index );
            OutputDebugString( debug_string );
            
            if ( cfg_enabled )
            {
                history_add( index );
                copy_master();
            }
            
            return ft_index;
        }
        
        // If plugin is not enabled, let Winamp shuffle
        if ( !cfg_enabled )
        {
            return -1;
        }
        
        playlist_wnd = get_playlist_hwnd();
        
        pl_len = SendMessage( hwnd, WM_WA_IPC, 0, IPC_GETLISTLENGTH );
        
        file.fileindex = index; // This is zero indexed
        ret = SendMessage( playlist_wnd, WM_WA_IPC, IPC_PE_GETINDEXTITLE, ( LPARAM ) & file );
        
        // If it returns 0 then track information was received
        if ( !ret )
        {
            // Check the track type to determine our behavior
            
            int type = atrack_type( index );
            
            // Get playing status
            status = SendMessage( hwnd, WM_WA_IPC, 0, IPC_ISPLAYING );

            // Pretend it is paused if anything but playing. Don't include separators because they are always stopped.
            is_pause = ( status != IS_PLAY && type != IS_ALBUM_SEP && type != IS_RANDOM_SEP );
            
            if ( is_pause )
                OutputDebugString( "Next track: Is paused" );
                
            wsprintf( debug_string, "Next track: Track type: %d", type );
            OutputDebugString( debug_string );
            
            if ( ( type == IS_ALBUM || type == IS_ALBUM_SEP ) && index < pl_len - 1 )
            {
                /* Track is an album separator or is in an album. Do not shuffle. Return
                 * the next in the playlist */
                OutputDebugString( "In an album:" );
                wsprintf( debug_string, "Old index: %d", index );
                OutputDebugString( debug_string );
                
                if ( type == IS_ALBUM )
                    history_add( index );
                    
                return ( index + 1 ) % pl_len;
            }
            else
            {
                /* Track is a random separator, the end of an album, the last album separator,
				 * or in a random section.
                 * Select a random track from the remaining list */
                is_shuffle = SendMessage( hwnd, WM_WA_IPC, 0, IPC_GET_SHUFFLE );
                
                if ( is_shuffle )
                {
                    int rand_i;
                    
                    // Return random number in interval [0 atrack_size()]
                    OutputDebugString( "Next track: Not in an album / exiting an album" );
                    
                    wsprintf( debug_string, "Next track: Items remaining: %d", atrack_size() );
                    OutputDebugString( debug_string );
                    
                    // Select random entry and play the entry
                    rand_i = ( unsigned int )( atrack_size() * genrand_real2() );
                    next = rm_atrack( rand_i );
                    
                    wsprintf( debug_string, "Next track: Random selected: %d", next );
                    OutputDebugString( debug_string );
                    
                    // If the list of random tracks is empty, copy from the master list
                    if ( atrack_size() == 0 )
                    {
                        OutputDebugString( "Next track: Track list empty, rebuilding the list" );
                        copy_master();
                    }
                    
                    // Don't add separators to the history
                    if ( type == IS_ALBUM_LAST || type == IS_RANDOM )
                        history_add( index );
                        
                    return next;
                }
                else
                    return -1;
            }
        }
        return -1;
    }
    
    return CallWindowProc( lpWndProcOld, hwnd, uMsg, wParam, lParam );
}


BOOL CALLBACK ConfigProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
            CheckDlgButton( hwndDlg, IDC_CHECK_ENABLE, cfg_enabled ? BST_CHECKED : BST_UNCHECKED );
            SetWindowText( hwndDlg, PLUGIN_DESC );
            break;
            
        case WM_COMMAND:
            if ( LOWORD( wParam ) == IDOK )
            {
                if ( IsDlgButtonChecked( hwndDlg, IDC_CHECK_ENABLE ) == BST_CHECKED )
                    cfg_enabled = 1;
                else
                    cfg_enabled = 0;
                EndDialog( hwndDlg, 0 );
                return TRUE;
            }
            if ( LOWORD( wParam ) == IDCANCEL )
            {
                EndDialog( hwndDlg, 0 );
                return TRUE;
            }
            break;
    }
    return FALSE;
}

// Read configuration from the ini file
void config_read()
{
    // ini_file=(char*)SendMessage(plugin.hwndParent,WM_USER,0,334);
    // if ((unsigned int)ini_file < 65536) ini_file="winamp.ini";
    
    cfg_enabled = GetPrivateProfileInt( PLUGIN_DESC, "CONFIG", cfg_enabled, ini_file );
}

// Write configuration to ini file
void config_write()
{
    char string[32];
    wsprintf( string, "%d", cfg_enabled );
    WritePrivateProfileString( PLUGIN_DESC, "CONFIG", string, ini_file );
}

HWND get_playlist_hwnd()
{
    HWND playlist_hwnd;
    // To get the playlist window there are two ways depending on the version you're using
    wa_version = SendMessage( plugin.hwndParent, WM_WA_IPC, 0, IPC_GETVERSION );
    if ( wa_version >= 0x2900 )
    {
        // use the built in api to get the handle
        playlist_hwnd = ( HWND )SendMessage( plugin.hwndParent, WM_WA_IPC, IPC_GETWND_PE, IPC_GETWND );
    }
    // if it failed then use the old way :o)
    if ( !playlist_hwnd )
    {
        playlist_hwnd = FindWindow( "Winamp PE", 0 );
    }
    return playlist_hwnd;
}

__declspec( dllexport ) winampGeneralPurposePlugin * winampGetGeneralPurposePlugin()
{
    return &plugin;
}
