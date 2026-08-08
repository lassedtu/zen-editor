#include "cursor.h"

/**
 * move the cursor up by one line, clamping to the buffer bounds
 * @param cur pointer to the cursor
 * @param buf pointer to the buffer
 */
void cursor_move_up(Cursor *cur, Buffer *buf)
{
    if (cur->row > 0)
    {
        cur->row--;
        cursor_clamp(cur, buf);
    }
}

/**
 * move the cursor down by one line, clamping to the buffer bounds
 * @param cur pointer to the cursor
 * @param buf pointer to the buffer
 */
void cursor_move_down(Cursor *cur, Buffer *buf)
{
    if (cur->row < buf->num_lines - 1)
    {
        cur->row++;
        cursor_clamp(cur, buf);
    }
}

/**
 * move the cursor left by one character, wrapping to the previous line if necessary
 * @param cur pointer to the cursor
 * @param buf pointer to the buffer
 */
void cursor_move_left(Cursor *cur, Buffer *buf)
{
    if (cur->col > 0)
    {
        cur->col--;
    }
    else if (cur->row > 0)
    {
        /* Wrap to end of previous line */
        cur->row--;
        cur->col = buf->lines[cur->row].len;
    }
}

/**
 * move the cursor right by one character, wrapping to the next line if necessary
 * @param cur pointer to the cursor
 * @param buf pointer to the buffer
 */
void cursor_move_right(Cursor *cur, Buffer *buf)
{
    if (cur->row < buf->num_lines)
    {
        Line *line = &buf->lines[cur->row];
        if (cur->col < line->len)
        {
            cur->col++;
        }
        else if (cur->row < buf->num_lines - 1)
        {
            /* Wrap to beginning of next line */
            cur->row++;
            cur->col = 0;
        }
    }
}

/**
 * move the cursor to the beginning of the current line
 * @param cur pointer to the cursor
 */
void cursor_home(Cursor *cur)
{
    cur->col = 0;
}

/**
 * move the cursor to the end of the current line
 * @param cur pointer to the cursor
 * @param buf pointer to the buffer
 */
void cursor_end(Cursor *cur, Buffer *buf)
{
    if (cur->row < buf->num_lines)
    {
        cur->col = buf->lines[cur->row].len;
    }
}

/**
 * clamp the cursor position to the bounds of the buffer
 * @param cur pointer to the cursor
 * @param buf pointer to the buffer
 */
void cursor_clamp(Cursor *cur, Buffer *buf)
{
    if (cur->row < 0)
        cur->row = 0;
    if (cur->row >= buf->num_lines)
        cur->row = buf->num_lines - 1;

    int line_len = buf->lines[cur->row].len;
    if (cur->col > line_len)
        cur->col = line_len;
    if (cur->col < 0)
        cur->col = 0;
}
