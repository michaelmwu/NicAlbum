/* Awesome header file */

// Prevent double inclusion
#ifndef ASHUFFLE_H_
#define ASHUFFLE_H_

#include <windows.h>
#include <stdlib.h>

#include "gen.h"
#include "resource.h"

#include "wa_ipc.h"
#include "ipc_pe.h"

#define PLUGIN_DESC "Nic's Album Shuffle"

#define VECT_SIZE 32677 // int
#define VECT_SIZE_MINUS_1 32676 // int

// Commands
#define WINAMP_PAUSE	40046
#define WINAMP_STOP		40047

// ISPLAYING status
#define IS_PLAY		1
#define IS_PAUSE	3
#define IS_STOP		0

void config();
void quit();
int init();

void config_read();
void config_write();

void build_list();

HWND get_playlist_hwnd();

#define DEBUG_DUMP	0

#endif
