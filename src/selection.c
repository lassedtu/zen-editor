#include "selection.h"

#include <stdlib.h>
#include <string.h>

/**
 * @file selection.c
 * @brief text selection state and region operations implementation.
 *
 * the region deletion reuses the low-level buffer operations that the editing
 * commands already use. it removes the region with one backspace-style pass
 * from the end point to the start point. each low-level step yields the inverse
 * operation that restores it. the pass records all inverse operations as one
 * grouped undo entry, so one undo rebuilds the full region in the correct
 * order.
 */

void selection_clear(Selection *sel)
{
    sel->active = 0;
    sel->anchor_row = 0;
    sel->anchor_col = 0;
    sel->cursor_row = 0;
    sel->cursor_col = 0;
}

void selection_start(Selection *sel, int row, int col)
{
    sel->active = 1;
    sel->anchor_row = row;
    sel->anchor_col = col;
    sel->cursor_row = row;
    sel->cursor_col = col;
}

void selection_set_cursor(Selection *sel, int row, int col)
{
    if (!sel->active)
        return;

    sel->cursor_row = row;
    sel->cursor_col = col;
}

/**
 * @brief test whether point a comes before point b in the buffer.
 * @param a_row row of the first point.
 * @param a_col column of the first point.
 * @param b_row row of the second point.
 * @param b_col column of the second point.
 * @return 1 if point a comes before point b, 0 otherwise.
 */
static int point_before(int a_row, int a_col, int b_row, int b_col)
{
    if (a_row != b_row)
        return a_row < b_row;
    return a_col < b_col;
}

void selection_normalize(const Selection *sel, int *start_row, int *start_col,
                         int *end_row, int *end_col)
{
    if (point_before(sel->anchor_row, sel->anchor_col,
                     sel->cursor_row, sel->cursor_col))
    {
        *start_row = sel->anchor_row;
        *start_col = sel->anchor_col;
        *end_row = sel->cursor_row;
        *end_col = sel->cursor_col;
    }
    else
    {
        *start_row = sel->cursor_row;
        *start_col = sel->cursor_col;
        *end_row = sel->anchor_row;
        *end_col = sel->anchor_col;
    }
}

int selection_contains(const Selection *sel, int row, int col)
{
    if (!sel->active)
        return 0;

    int sr, sc, er, ec;
    selection_normalize(sel, &sr, &sc, &er, &ec);

    /* before the start point: not inside */
    if (point_before(row, col, sr, sc))
        return 0;

    /* at or after the end point: not inside (half-open range) */
    if (!point_before(row, col, er, ec))
        return 0;

    return 1;
}

/**
 * @brief test whether the normalized region is empty (start equals end).
 * @param sel pointer to the Selection to test.
 * @return 1 if the region is empty, 0 otherwise.
 */
static int region_is_empty(const Selection *sel)
{
    int sr, sc, er, ec;
    selection_normalize(sel, &sr, &sc, &er, &ec);
    return sr == er && sc == ec;
}

char *selection_copy_region(const Selection *sel, const Buffer *buf,
                            int *out_len)
{
    if (!sel->active || region_is_empty(sel))
    {
        if (out_len)
            *out_len = 0;
        return NULL;
    }

    int sr, sc, er, ec;
    selection_normalize(sel, &sr, &sc, &er, &ec);

    /* compute the total length: line spans plus one newline per line break */
    int total = 0;
    if (sr == er)
    {
        total = ec - sc;
    }
    else
    {
        total += buf->lines[sr].len - sc; /* tail of the first line */
        total += 1;                       /* newline after the first line */
        for (int r = sr + 1; r < er; r++)
        {
            total += buf->lines[r].len; /* whole middle line */
            total += 1;                 /* newline after it */
        }
        total += ec; /* head of the last line */
    }

    char *text = malloc(total + 1);
    if (!text)
    {
        if (out_len)
            *out_len = 0;
        return NULL;
    }

    int pos = 0;
    if (sr == er)
    {
        memcpy(&text[pos], &buf->lines[sr].chars[sc], ec - sc);
        pos += ec - sc;
    }
    else
    {
        int first_tail = buf->lines[sr].len - sc;
        if (first_tail > 0)
        {
            memcpy(&text[pos], &buf->lines[sr].chars[sc], first_tail);
            pos += first_tail;
        }
        text[pos++] = '\n';

        for (int r = sr + 1; r < er; r++)
        {
            if (buf->lines[r].len > 0)
            {
                memcpy(&text[pos], buf->lines[r].chars, buf->lines[r].len);
                pos += buf->lines[r].len;
            }
            text[pos++] = '\n';
        }

        if (ec > 0)
        {
            memcpy(&text[pos], buf->lines[er].chars, ec);
            pos += ec;
        }
    }

    text[pos] = '\0';
    if (out_len)
        *out_len = pos;
    return text;
}

int selection_delete_region(const Selection *sel, Buffer *buf, History *hist,
                            int *out_row, int *out_col)
{
    if (!sel->active || region_is_empty(sel))
        return 0;

    int sr, sc, er, ec;
    selection_normalize(sel, &sr, &sc, &er, &ec);

    /* record inverse operations in the order the deletions happen. the pass
       walks backward from the end point to the start point, exactly like
       repeated Backspace, so every step stays in bounds. entry_apply replays
       the group in reverse, which restores the region in the correct order. */
    UndoOp *ops = NULL;
    int num_ops = 0;
    int cap = 0;

    int row = er;
    int col = ec;

    while (point_before(sr, sc, row, col))
    {
        UndoOp op;

        if (col > 0)
        {
            /* delete the character before the current column */
            char deleted = buf->lines[row].chars[col - 1];
            buffer_delete_char(buf, row, col - 1);
            op.type = UNDO_OP_INSERT_CHAR;
            op.row = row;
            op.col = col - 1;
            op.ch = deleted;
            col--;
        }
        else
        {
            /* at column 0: merge this line into the previous one */
            int prev_len = buf->lines[row - 1].len;
            buffer_delete_line(buf, row);
            op.type = UNDO_OP_SPLIT_LINE;
            op.row = row - 1;
            op.col = prev_len;
            op.ch = 0;
            row--;
            col = prev_len;
        }

        if (num_ops == cap)
        {
            cap = cap ? cap * 2 : 16;
            ops = realloc(ops, sizeof(UndoOp) * cap);
        }
        ops[num_ops++] = op;
    }

    /* the pre-edit cursor sits at the end point; the post-edit cursor sits at
       the start point, where the region collapsed to */
    history_record_group(hist, ops, num_ops, er, ec, sr, sc);
    free(ops);

    *out_row = sr;
    *out_col = sc;
    return 1;
}
