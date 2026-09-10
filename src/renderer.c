#include "renderer.h"

#include "platform_terminal.h"
#include "search.h"

#include <stdio.h>
#include <string.h>

/* ANSI attribute sequences for inverted-video match highlighting */
#define ZE_INVERT_ON "\x1b[7m"
#define ZE_INVERT_OFF "\x1b[27m"

/**
 * @brief draw a single line, inverting any spans that match the query.
 *
 * emits the visible portion of the line up to screen_cols. when a query is
 * supplied, every non-overlapping occurrence within that portion is wrapped in
 * inverted-video escapes so matches stand out.
 *
 * @param line pointer to the line to draw.
 * @param screen_cols maximum number of columns to draw.
 * @param query optional query string to highlight; NULL or empty draws plainly.
 */
static void draw_line_highlighted(const Line *line, int screen_cols,
                                  const char *query)
{
    int len = line->len;
    if (len > screen_cols)
    {
        len = screen_cols;
    }
    if (len <= 0)
    {
        return;
    }

    int qlen = (query != NULL) ? (int)strlen(query) : 0;
    if (qlen == 0)
    {
        platform_terminal_write(line->chars, len);
        return;
    }

    int col = 0;
    while (col < len)
    {
        int match_col;
        if (search_find_in_line(line, query, col, &match_col) &&
            match_col + qlen <= len)
        {
            /* plain text before the match */
            if (match_col > col)
            {
                platform_terminal_write(line->chars + col, match_col - col);
            }
            /* the highlighted match itself */
            platform_terminal_write(ZE_INVERT_ON, (int)strlen(ZE_INVERT_ON));
            platform_terminal_write(line->chars + match_col, qlen);
            platform_terminal_write(ZE_INVERT_OFF, (int)strlen(ZE_INVERT_OFF));
            col = match_col + qlen;
        }
        else
        {
            /* no further match on this line: emit the remainder */
            platform_terminal_write(line->chars + col, len - col);
            break;
        }
    }
}

void renderer_draw(Buffer *buf, Cursor *cur, int screen_rows, int screen_cols,
                   int scroll_offset, const char *highlight)
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
            draw_line_highlighted(&buf->lines[file_row], screen_cols, highlight);
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

void renderer_draw_search_prompt(const char *query, int match_count,
                                 int screen_rows, int screen_cols)
{
    (void)screen_cols;

    char prompt[256];
    int len = snprintf(prompt, sizeof(prompt), " Search: %s (%d matches)",
                       query ? query : "", match_count);

    platform_terminal_move_cursor(screen_rows - 1, 0);

    if (len > 255)
        len = 255;
    platform_terminal_write(prompt, len);
}
