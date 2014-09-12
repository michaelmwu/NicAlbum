/* Header file for the history list
 * History is a stack, except when the max capacity is reached, it
 * overwrites the bottom of the stack */

// Prevent double inclusion
#ifndef THISTORY_H_
#define THISTORY_H_

// History
#define HISTORY_SIZE 64

typedef struct t_history_s
{
	int size;
	int cur;
	int tracks[HISTORY_SIZE];
} t_history;

int history_size();

int history_pop();
void history_add(int index);

void history_clear();

#endif
