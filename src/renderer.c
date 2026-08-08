#include "renderer.h"
#include "platform_terminal.h"
#include <stdio.h>
#include <string.h>

void renderer_draw(Buffer *buf, Cursor *cur, int screen_rows, int screen_cols,
                   int scroll_offset)
{
    /* hide cursor during redraw */
    platform_terminal_write("\x1b[?25l", 6);
    platform_terminal_clear();

    /* reserve the last row for the status bar */
    int draw_rows = screen_rows - 1;

    for (int i = 0; i < draw_rows; i++)
    {
        int file_row = i + scroll_offset;
        platform_terminal_move_cursor(i, 0);

        if (file_row < buf->num_lines)
        {
            Line *line = &buf->lines[file_row];
            int len = line->len;
            if (len > screen_cols)
                len = screen_cols;
            if (len > 0)
            {
                platform_terminal_write(line->chars, len);
            }
        }
        else
        {
            platform_terminal_write("~", 1);
        }
    }

    /* position the cursor and show it */
    platform_terminal_move_cursor(cur->row - scroll_offset, cur->col);
    platform_terminal_write("\x1b[?25h", 6);
}

void renderer_draw_status(const char *filename, int num_lines, int cur_row,
                          int screen_rows)
{
    char status[256];
    int len = snprintf(status, sizeof(status), " %s | %d lines | row %d",
                       filename ? filename : "[No Name]", num_lines, cur_row + 1);

    platform_terminal_move_cursor(screen_rows - 1, 0);

    /* write status with a limited length */
    if (len > 255)
        len = 255;
    platform_terminal_write(status, len);
}
