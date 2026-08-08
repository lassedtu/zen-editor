#ifndef ZE_RENDERER_H
#define ZE_RENDERER_H

#include "buffer.h"
#include "cursor.h"

/*
 * Renderer
 *
 * Draws the buffer contents to the terminal.
 */

/* Draw the entire visible portion of the buffer */
void renderer_draw(Buffer *buf, Cursor *cur, int screen_rows, int screen_cols,
                   int scroll_offset);

/* Draw the status bar at the bottom of the screen */
void renderer_draw_status(const char *filename, int num_lines, int cur_row,
                          int screen_rows);

#endif /* ZE_RENDERER_H */
