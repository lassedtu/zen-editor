#include "search.h"

#include <string.h>

/**
 * @file search.c
 * @brief buffer search implementation.
 *
 * matches are found with a plain substring scan (no regex yet). the forward
 * and backward walkers share the same per-line primitive and both wrap around
 * the buffer exactly once so every position is visited at most once.
 */

/**
 * @brief test whether query occurs in line->chars starting exactly at col.
 * @param line pointer to the line to test.
 * @param query the string to look for.
 * @param qlen length of the query.
 * @param col column at which to test for a match.
 * @return 1 if the query matches at col, 0 otherwise.
 */
static int matches_at(const Line *line, const char *query, int qlen, int col)
{
    if (col < 0 || col + qlen > line->len)
    {
        return 0;
    }
    return memcmp(line->chars + col, query, (size_t)qlen) == 0;
}

int search_find_in_line(const Line *line, const char *query, int from_col,
                        int *out_col)
{
    if (line == NULL || query == NULL)
    {
        return 0;
    }

    int qlen = (int)strlen(query);
    if (qlen == 0)
    {
        return 0;
    }

    int start = from_col < 0 ? 0 : from_col;
    for (int col = start; col + qlen <= line->len; col++)
    {
        if (matches_at(line, query, qlen, col))
        {
            if (out_col != NULL)
            {
                *out_col = col;
            }
            return 1;
        }
    }
    return 0;
}

int search_count_in_line(const Line *line, const char *query)
{
    if (line == NULL || query == NULL)
    {
        return 0;
    }

    int qlen = (int)strlen(query);
    if (qlen == 0)
    {
        return 0;
    }

    int count = 0;
    int col = 0;
    while (search_find_in_line(line, query, col, &col))
    {
        count++;
        col += qlen; /* non-overlapping matches */
    }
    return count;
}

/**
 * @brief find the last match on a line that begins at or before a column.
 * @param line pointer to the line to scan.
 * @param query the string to look for.
 * @param qlen length of the query.
 * @param max_col highest column a match may start at (inclusive).
 * @param out_col pointer that receives the match column on success.
 * @return 1 if a match was found, 0 otherwise.
 */
static int find_last_in_line(const Line *line, const char *query, int qlen,
                             int max_col, int *out_col)
{
    if (max_col > line->len - qlen)
    {
        max_col = line->len - qlen;
    }
    for (int col = max_col; col >= 0; col--)
    {
        if (matches_at(line, query, qlen, col))
        {
            if (out_col != NULL)
            {
                *out_col = col;
            }
            return 1;
        }
    }
    return 0;
}

int search_find_next(const Buffer *buf, const char *query, int from_row,
                     int from_col, SearchMatch *out)
{
    if (buf == NULL || query == NULL || buf->num_lines == 0)
    {
        return 0;
    }

    int qlen = (int)strlen(query);
    if (qlen == 0)
    {
        return 0;
    }

    /* clamp the starting row into range so callers can pass loose coordinates */
    if (from_row < 0)
    {
        from_row = 0;
        from_col = 0;
    }
    if (from_row >= buf->num_lines)
    {
        from_row = 0;
        from_col = 0;
    }

    /* visit num_lines + 1 rows so the origin row is re-checked after wrapping */
    for (int i = 0; i <= buf->num_lines; i++)
    {
        int row = (from_row + i) % buf->num_lines;
        int start = (i == 0) ? from_col : 0;

        /* on the wrapped return to the origin row, stop before the start col */
        int col;
        if (search_find_in_line(&buf->lines[row], query, start, &col))
        {
            if (i == buf->num_lines && col >= from_col)
            {
                break;
            }
            if (out != NULL)
            {
                out->row = row;
                out->col = col;
                out->len = qlen;
            }
            return 1;
        }
    }
    return 0;
}

int search_find_prev(const Buffer *buf, const char *query, int from_row,
                     int from_col, SearchMatch *out)
{
    if (buf == NULL || query == NULL || buf->num_lines == 0)
    {
        return 0;
    }

    int qlen = (int)strlen(query);
    if (qlen == 0)
    {
        return 0;
    }

    if (from_row < 0 || from_row >= buf->num_lines)
    {
        from_row = buf->num_lines - 1;
        from_col = buf->lines[from_row].len;
    }

    /* visit num_lines + 1 rows so the origin row is re-checked after wrapping */
    for (int i = 0; i <= buf->num_lines; i++)
    {
        int row = ((from_row - i) % buf->num_lines + buf->num_lines) % buf->num_lines;

        /* on the origin row, only consider matches starting before from_col;
         * on all other rows, any starting column is allowed */
        int max_col = (i == 0) ? from_col - 1 : buf->lines[row].len;

        int col;
        if (find_last_in_line(&buf->lines[row], query, qlen, max_col, &col))
        {
            if (i == buf->num_lines && col < from_col)
            {
                break;
            }
            if (out != NULL)
            {
                out->row = row;
                out->col = col;
                out->len = qlen;
            }
            return 1;
        }
    }
    return 0;
}
