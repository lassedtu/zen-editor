#include "renderer.h"

#include "platform_terminal.h"
#include "search.h"
#include "selection.h"

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

/**
 * @brief draw a single line, inverting the part that lies in the selection.
 *
 * draws the characters of the line up to screen_cols. the characters whose
 * columns lie inside the selection region are wrapped in inverted-video
 * escapes. the function computes the selected span on this row from the
 * normalized region: on the first row the span starts at the region start
 * column, on the last row it ends at the region end column, and full middle
 * rows are selected end to end.
 *
 * @param line pointer to the line to draw.
 * @param file_row the buffer row index of this line.
 * @param screen_cols maximum number of columns to draw.
 * @param sel pointer to the active selection.
 */
static void draw_line_selected(const Line *line, int file_row, int screen_cols,
                               const Selection *sel)
{
    int len = line->len;
    if (len > screen_cols)
    {
        len = screen_cols;
    }

    int sr, sc, er, ec;
    selection_normalize(sel, &sr, &sc, &er, &ec);

    /* compute the selected column span [sel_start, sel_end) on this row */
    int sel_start = (file_row == sr) ? sc : 0;
    int sel_end = (file_row == er) ? ec : len;

    if (sel_start < 0)
        sel_start = 0;
    if (sel_end > len)
        sel_end = len;

    /* plain text before the selected span */
    if (sel_start > 0)
    {
        platform_terminal_write(line->chars, sel_start);
    }

    /* the selected span, drawn with inverted video */
    if (sel_end > sel_start)
    {
        platform_terminal_write(ZE_INVERT_ON, (int)strlen(ZE_INVERT_ON));
        platform_terminal_write(line->chars + sel_start, sel_end - sel_start);
        platform_terminal_write(ZE_INVERT_OFF, (int)strlen(ZE_INVERT_OFF));
    }

    /* plain text after the selected span */
    if (sel_end < len)
    {
        platform_terminal_write(line->chars + sel_end, len - sel_end);
    }
}

/**
 * @brief test whether the selection touches a given buffer row.
 * @param sel pointer to the selection.
 * @param file_row the buffer row index to test.
 * @return 1 if the row is within the normalized region rows, 0 otherwise.
 */
static int selection_touches_row(const Selection *sel, int file_row)
{
    if (sel == NULL || !sel->active)
        return 0;

    int sr, sc, er, ec;
    selection_normalize(sel, &sr, &sc, &er, &ec);
    (void)sc;
    (void)ec;
    return file_row >= sr && file_row <= er;
}

void renderer_draw(Buffer *buf, Cursor *cur, int screen_rows, int screen_cols,
                   int scroll_offset, const char *highlight,
                   const Selection *sel)
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
            if (selection_touches_row(sel, file_row))
            {
                draw_line_selected(&buf->lines[file_row], file_row,
                                   screen_cols, sel);
            }
            else
            {
                draw_line_highlighted(&buf->lines[file_row], screen_cols,
                                      highlight);
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
