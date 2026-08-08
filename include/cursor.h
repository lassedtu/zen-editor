#ifndef ZE_CURSOR_H
#define ZE_CURSOR_H

#include "buffer.h"

/*
 * Cursor
 *
 * Manages the editing position within a buffer.
 */

typedef struct {
    int row;
    int col;
} Cursor;

/* Move the cursor in the given direction, clamped to buffer bounds */
void cursor_move_up(Cursor *cur, Buffer *buf);
void cursor_move_down(Cursor *cur, Buffer *buf);
void cursor_move_left(Cursor *cur, Buffer *buf);
void cursor_move_right(Cursor *cur, Buffer *buf);

/* Move to the beginning/end of the current line */
void cursor_home(Cursor *cur);
void cursor_end(Cursor *cur, Buffer *buf);

/* Clamp cursor position to valid buffer coordinates */
void cursor_clamp(Cursor *cur, Buffer *buf);

#endif /* ZE_CURSOR_H */
