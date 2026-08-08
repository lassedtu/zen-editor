#include "buffer.h"
#include "platform_fs.h"
#include <stdlib.h>
#include <string.h>

Buffer *buffer_create(void)
{
    Buffer *buf = malloc(sizeof(Buffer));
    if (!buf)
        return NULL;

    /* start with one empty line */
    buf->lines = malloc(sizeof(Line));
    buf->lines[0].chars = NULL;
    buf->lines[0].len = 0;
    buf->num_lines = 1;
    buf->modified = 0;

    return buf;
}

void buffer_free(Buffer *buf)
{
    if (!buf)
        return;

    for (int i = 0; i < buf->num_lines; i++)
    {
        free(buf->lines[i].chars);
    }
    free(buf->lines);
    free(buf);
}

void buffer_insert_char(Buffer *buf, int row, int col, char c)
{
    if (row < 0 || row >= buf->num_lines)
        return;

    Line *line = &buf->lines[row];
    if (col < 0 || col > line->len)
        return;

    line->chars = realloc(line->chars, line->len + 1);
    /* shift characters to the right */
    memmove(&line->chars[col + 1], &line->chars[col], line->len - col);
    line->chars[col] = c;
    line->len++;
    buf->modified = 1;
}

void buffer_delete_char(Buffer *buf, int row, int col)
{
    if (row < 0 || row >= buf->num_lines)
        return;

    Line *line = &buf->lines[row];
    if (col < 0 || col >= line->len)
        return;

    /* shift characters to the left */
    memmove(&line->chars[col], &line->chars[col + 1], line->len - col - 1);
    line->len--;
    line->chars = realloc(line->chars, line->len);
    buf->modified = 1;
}

void buffer_insert_newline(Buffer *buf, int row, int col)
{
    if (row < 0 || row >= buf->num_lines)
        return;
    if (col < 0 || col > buf->lines[row].len)
        return;

    /* save the data we need before realloc potentially moves the array */
    int old_len = buf->lines[row].len;
    int new_len = old_len - col;

    /* copy the tail of the current line (text after the cursor) */
    char *tail = NULL;
    if (new_len > 0)
    {
        tail = malloc(new_len);
        memcpy(tail, &buf->lines[row].chars[col], new_len);
    }

    /* make room for a new line */
    buf->lines = realloc(buf->lines, sizeof(Line) * (buf->num_lines + 1));

    /* shift lines down */
    memmove(&buf->lines[row + 2], &buf->lines[row + 1],
            sizeof(Line) * (buf->num_lines - row - 1));

    /* set up the new line with the tail content */
    buf->lines[row + 1].len = new_len;
    buf->lines[row + 1].chars = tail;

    /* truncate the original line */
    buf->lines[row].len = col;
    if (col > 0)
    {
        buf->lines[row].chars = realloc(buf->lines[row].chars, col);
    }
    else
    {
        free(buf->lines[row].chars);
        buf->lines[row].chars = NULL;
    }

    buf->num_lines++;
    buf->modified = 1;
}

void buffer_delete_line(Buffer *buf, int row)
{
    if (row <= 0 || row >= buf->num_lines)
        return;

    /* merge this line into the previous one */
    Line *prev = &buf->lines[row - 1];
    Line *cur = &buf->lines[row];

    int new_len = prev->len + cur->len;
    prev->chars = realloc(prev->chars, new_len);
    if (cur->len > 0)
    {
        memcpy(&prev->chars[prev->len], cur->chars, cur->len);
    }
    prev->len = new_len;

    free(cur->chars);

    /* shift lines up */
    memmove(&buf->lines[row], &buf->lines[row + 1],
            sizeof(Line) * (buf->num_lines - row - 1));
    buf->num_lines--;
    buf->lines = realloc(buf->lines, sizeof(Line) * buf->num_lines);
    buf->modified = 1;
}

int buffer_load(Buffer *buf, const char *filename)
{
    int file_len = 0;
    char *data = platform_fs_read_file(filename, &file_len);
    if (!data)
        return -1;

    /* free existing lines */
    for (int i = 0; i < buf->num_lines; i++)
    {
        free(buf->lines[i].chars);
    }
    free(buf->lines);

    /* count lines */
    int line_count = 1;
    for (int i = 0; i < file_len; i++)
    {
        if (data[i] == '\n')
            line_count++;
    }

    buf->lines = malloc(sizeof(Line) * line_count);
    buf->num_lines = line_count;

    int line_idx = 0;
    int line_start = 0;
    for (int i = 0; i <= file_len; i++)
    {
        if (i == file_len || data[i] == '\n')
        {
            int len = i - line_start;
            buf->lines[line_idx].len = len;
            buf->lines[line_idx].chars = NULL;
            if (len > 0)
            {
                buf->lines[line_idx].chars = malloc(len);
                memcpy(buf->lines[line_idx].chars, &data[line_start], len);
            }
            line_idx++;
            line_start = i + 1;
        }
    }

    free(data);
    buf->modified = 0;
    return 0;
}

int buffer_save(Buffer *buf, const char *filename)
{
    /* calculate total size needed */
    int total = 0;
    for (int i = 0; i < buf->num_lines; i++)
    {
        total += buf->lines[i].len;
        if (i < buf->num_lines - 1)
            total++; /* newline */
    }

    char *data = malloc(total);
    int pos = 0;
    for (int i = 0; i < buf->num_lines; i++)
    {
        if (buf->lines[i].len > 0)
        {
            memcpy(&data[pos], buf->lines[i].chars, buf->lines[i].len);
            pos += buf->lines[i].len;
        }
        if (i < buf->num_lines - 1)
        {
            data[pos++] = '\n';
        }
    }

    int result = platform_fs_write_file(filename, data, total);
    free(data);

    if (result == 0)
    {
        buf->modified = 0;
    }
    return result;
}
