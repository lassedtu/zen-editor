#include "cursor.h"

void cursor_move_up(Cursor *cur, Buffer *buf)
{
    if (cur->row > 0)
    {
        cur->row--;
        cursor_clamp(cur, buf);
    }
}

void cursor_move_down(Cursor *cur, Buffer *buf)
{
    if (cur->row < buf->num_lines - 1)
    {
        cur->row++;
        cursor_clamp(cur, buf);
    }
}

void cursor_move_left(Cursor *cur, Buffer *buf)
{
    if (cur->col > 0)
    {
        cur->col--;
    }
    else if (cur->row > 0)
    {
        /* wrap to end of previous line */
        cur->row--;
        cur->col = buf->lines[cur->row].len;
    }
}

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
            /* wrap to beginning of next line */
            cur->row++;
            cur->col = 0;
        }
    }
}

void cursor_home(Cursor *cur)
{
    cur->col = 0;
}

void cursor_end(Cursor *cur, Buffer *buf)
{
    if (cur->row < buf->num_lines)
    {
        cur->col = buf->lines[cur->row].len;
    }
}

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
