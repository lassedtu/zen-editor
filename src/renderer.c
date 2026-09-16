#include "renderer.h"

#include "platform_terminal.h"
#include "search.h"
#include "selection.h"
#include "config.h"

#include <stdio.h>
#include <string.h>

/* ANSI attribute sequences for inverted-video match highlighting */
#define ZE_INVERT_ON "\x1b[7m"
#define ZE_INVERT_OFF "\x1b[27m"

/* ANSI sequence that returns all attributes and colors to the default */
#define ZE_RESET "\x1b[0m"

/**
 * @brief write a color start sequence when the color value is not empty.
 *
 * the function makes an ANSI Select Graphic Rendition sequence from the value.
 * an example is "\x1b[31m" from the value "31". the function writes nothing
 * when the value is empty. an empty value keeps the color of the terminal.
 *
 * @param color the numeric part of the color, as a string.
 */
static void write_color_on(const char *color)
{
    if (color == NULL || color[0] == '\0')
    {
        return;
    }

    char seq[32];
    int len = snprintf(seq, sizeof(seq), "\x1b[%sm", color);
    if (len > 0)
    {
        platform_terminal_write(seq, len);
    }
}

/**
 * @brief write the reset sequence when the color value is not empty.
 *
 * the function writes the reset sequence to end a color. it writes nothing when
 * the value is empty, because then the code wrote no color start before.
 *
 * @param color the color value that the code used for the start.
 */
static void write_color_off(const char *color)
{
    if (color == NULL || color[0] == '\0')
    {
        return;
    }
    platform_terminal_write(ZE_RESET, (int)strlen(ZE_RESET));
}

/**
 * @brief write a run of characters and change each tab to spaces.
 *
 * the function writes the characters one by one. it writes a normal character
 * as it is. it changes a tab to the number of spaces that fill the line to the
 * next tab stop. the tab stop distance is the tab size. the function uses the
 * visual column to find the next tab stop, and it changes the visual column for
 * each character that it writes.
 *
 * @param chars pointer to the first character of the run.
 * @param len number of characters in the run.
 * @param tab_size number of columns for one tab stop.
 * @param vcol pointer to the current visual column; the function changes it.
 */
static void write_expanded(const char *chars, int len, int tab_size,
                           int *vcol)
{
    static const char spaces[16] = "                ";

    for (int i = 0; i < len; i++)
    {
        if (chars[i] == '\t')
        {
            int width = tab_size;
            if (width < 1)
            {
                width = 1;
            }

            int next_stop = ((*vcol / width) + 1) * width;
            int fill = next_stop - *vcol;
            while (fill > 0)
            {
                int chunk = (fill > 16) ? 16 : fill;
                platform_terminal_write(spaces, chunk);
                fill -= chunk;
            }
            *vcol = next_stop;
        }
        else
        {
            platform_terminal_write(&chars[i], 1);
            (*vcol)++;
        }
    }
}

/**
 * @brief draw a single line, inverting any spans that match the query.
 *
 * the function writes the visible part of the line up to screen_cols. when a
 * query is there, the function puts inverted-video escapes around each match.
 * the function changes each tab to spaces of the tab width.
 *
 * @param line pointer to the line to draw.
 * @param screen_cols maximum number of columns to draw.
 * @param query optional query string to highlight; NULL or empty draws plainly.
 * @param tab_size number of columns for one tab stop.
 * @param highlight_color the color for a match; empty uses inverted video only.
 */
static void draw_line_highlighted(const Line *line, int screen_cols,
                                  const char *query, int tab_size,
                                  const char *highlight_color)
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
    int vcol = 0;
    if (qlen == 0)
    {
        write_expanded(line->chars, len, tab_size, &vcol);
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
                write_expanded(line->chars + col, match_col - col, tab_size,
                               &vcol);
            }
            /* the match, with the highlight color and inverted video */
            write_color_on(highlight_color);
            platform_terminal_write(ZE_INVERT_ON, (int)strlen(ZE_INVERT_ON));
            write_expanded(line->chars + match_col, qlen, tab_size, &vcol);
            platform_terminal_write(ZE_INVERT_OFF, (int)strlen(ZE_INVERT_OFF));
            write_color_off(highlight_color);
            col = match_col + qlen;
        }
        else
        {
            /* no more match on this line: write the rest */
            write_expanded(line->chars + col, len - col, tab_size, &vcol);
            break;
        }
    }
}

/**
 * @brief draw a single line, inverting the part that lies in the selection.
 *
 * the function writes the characters of the line up to screen_cols. it puts
 * inverted-video escapes around the characters that lie in the selection. the
 * function finds the selected columns from the normalized region. on the first
 * row the span starts at the region start column. on the last row it ends at
 * the region end column. a full middle row is selected from end to end. the
 * function changes each tab to spaces of the tab width.
 *
 * @param line pointer to the line to draw.
 * @param file_row the buffer row index of this line.
 * @param screen_cols maximum number of columns to draw.
 * @param sel pointer to the active selection.
 * @param tab_size number of columns for one tab stop.
 */
static void draw_line_selected(const Line *line, int file_row, int screen_cols,
                               const Selection *sel, int tab_size)
{
    int len = line->len;
    if (len > screen_cols)
    {
        len = screen_cols;
    }

    int sr, sc, er, ec;
    selection_normalize(sel, &sr, &sc, &er, &ec);

    /* find the selected columns [sel_start, sel_end) on this row */
    int sel_start = (file_row == sr) ? sc : 0;
    int sel_end = (file_row == er) ? ec : len;

    if (sel_start < 0)
        sel_start = 0;
    if (sel_end > len)
        sel_end = len;

    int vcol = 0;

    /* plain text before the selected span */
    if (sel_start > 0)
    {
        write_expanded(line->chars, sel_start, tab_size, &vcol);
    }

    /* the selected span, with inverted video */
    if (sel_end > sel_start)
    {
        platform_terminal_write(ZE_INVERT_ON, (int)strlen(ZE_INVERT_ON));
        write_expanded(line->chars + sel_start, sel_end - sel_start, tab_size,
                       &vcol);
        platform_terminal_write(ZE_INVERT_OFF, (int)strlen(ZE_INVERT_OFF));
    }

    /* plain text after the selected span */
    if (sel_end < len)
    {
        write_expanded(line->chars + sel_end, len - sel_end, tab_size, &vcol);
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
                   const Selection *sel, const Config *cfg)
{
    int tab_size = (cfg != NULL) ? cfg->tab_size : 4;
    const char *fg = (cfg != NULL) ? cfg->theme.foreground : "";
    const char *hl = (cfg != NULL) ? cfg->theme.search_highlight : "";

    /* hide cursor during redraw */
    platform_terminal_write("\x1b[?25l", 6);
    platform_terminal_clear();

    /* set the text color for the whole draw */
    write_color_on(fg);

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
                                   screen_cols, sel, tab_size);
            }
            else
            {
                draw_line_highlighted(&buf->lines[file_row], screen_cols,
                                      highlight, tab_size, hl);
            }
        }
        else
        {
            platform_terminal_write("~", 1);
        }
    }

    /* return the text color to the default */
    write_color_off(fg);

    /* position the cursor at the visual column and show it */
    int vcol = cur->col;
    if (cur->row >= 0 && cur->row < buf->num_lines)
    {
        vcol = renderer_visual_col(&buf->lines[cur->row], cur->col, tab_size);
    }
    platform_terminal_move_cursor(cur->row - scroll_offset, vcol);
    platform_terminal_write("\x1b[?25h", 6);
}

void renderer_draw_status(const char *filename, int num_lines, int cur_row,
                          int screen_rows, const Config *cfg)
{
    const char *color = (cfg != NULL) ? cfg->theme.status_bar : "";

    char status[256];
    int len = snprintf(status, sizeof(status), " %s | %d lines | row %d",
                       filename ? filename : "[No Name]", num_lines, cur_row + 1);

    platform_terminal_move_cursor(screen_rows - 1, 0);

    /* write status with a limited length */
    if (len > 255)
        len = 255;

    write_color_on(color);
    platform_terminal_write(status, len);
    write_color_off(color);
}

int renderer_visual_col(const Line *line, int col, int tab_size)
{
    int width = (tab_size >= 1) ? tab_size : 1;
    int vcol = 0;

    int limit = col;
    if (limit > line->len)
    {
        limit = line->len;
    }

    for (int i = 0; i < limit; i++)
    {
        if (line->chars[i] == '\t')
        {
            vcol = ((vcol / width) + 1) * width;
        }
        else
        {
            vcol++;
        }
    }

    return vcol;
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
