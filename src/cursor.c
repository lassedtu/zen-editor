#include "cursor.h"

/**
 * @brief character class used for word-wise movement.
 */
typedef enum
{
    CHAR_CLASS_SPACE, // space or tab
    CHAR_CLASS_WORD,  // letter, digit, or underscore
    CHAR_CLASS_PUNCT, // any other visible character
} CharClass;

/**
 * @brief return the word-motion class of a character.
 * @param c the character to classify.
 * @return the character class of c.
 */
static CharClass char_class(char c)
{
    if (c == ' ' || c == '\t')
        return CHAR_CLASS_SPACE;

    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '_')
        return CHAR_CLASS_WORD;

    return CHAR_CLASS_PUNCT;
}

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

void cursor_move_word_left(Cursor *cur, Buffer *buf)
{
    /* at the start of a line: wrap to the end of the previous line */
    if (cur->col == 0)
    {
        if (cur->row > 0)
        {
            cur->row--;
            cur->col = buf->lines[cur->row].len;
        }
        return;
    }

    const Line *line = &buf->lines[cur->row];

    /* skip whitespace immediately to the left */
    while (cur->col > 0 &&
           char_class(line->chars[cur->col - 1]) == CHAR_CLASS_SPACE)
    {
        cur->col--;
    }

    /* skip the run of characters of the same class as the one to the left */
    if (cur->col > 0)
    {
        CharClass run = char_class(line->chars[cur->col - 1]);
        while (cur->col > 0 && char_class(line->chars[cur->col - 1]) == run)
        {
            cur->col--;
        }
    }
}

void cursor_move_word_right(Cursor *cur, Buffer *buf)
{
    const Line *line = &buf->lines[cur->row];

    /* at the end of a line: wrap to the start of the next line */
    if (cur->col >= line->len)
    {
        if (cur->row < buf->num_lines - 1)
        {
            cur->row++;
            cur->col = 0;
        }
        return;
    }

    /* skip whitespace immediately to the right */
    while (cur->col < line->len &&
           char_class(line->chars[cur->col]) == CHAR_CLASS_SPACE)
    {
        cur->col++;
    }

    /* skip the run of characters of the same class as the one to the right */
    if (cur->col < line->len)
    {
        CharClass run = char_class(line->chars[cur->col]);
        while (cur->col < line->len && char_class(line->chars[cur->col]) == run)
        {
            cur->col++;
        }
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
