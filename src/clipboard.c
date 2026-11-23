#include "clipboard.h"

#include <stdlib.h>
#include <string.h>

/**
 * @file clipboard.c
 * @brief internal clipboard buffer and paste operation implementation.
 *
 * the paste reuses the low-level buffer operations that the editing commands
 * already use. it inserts the clipboard text one element at a time and records
 * the inverse of each insertion. the inverse operations form one grouped undo
 * entry, so one undo removes the whole pasted text in the correct order.
 */

void clipboard_init(Clipboard *cb)
{
    cb->text = NULL;
    cb->len = 0;
}

void clipboard_free(Clipboard *cb)
{
    free(cb->text);
    cb->text = NULL;
    cb->len = 0;
}

int clipboard_is_empty(const Clipboard *cb)
{
    return cb->text == NULL || cb->len == 0;
}

void clipboard_set(Clipboard *cb, const char *text, int len)
{
    free(cb->text);
    cb->text = NULL;
    cb->len = 0;

    if (text == NULL || len <= 0)
        return;

    cb->text = malloc(len + 1);
    if (cb->text == NULL)
        return;

    memcpy(cb->text, text, len);
    cb->text[len] = '\0';
    cb->len = len;
}

int clipboard_paste(const Clipboard *cb, Buffer *buf, History *hist,
                    int row, int col, int *out_row, int *out_col)
{
    if (clipboard_is_empty(cb))
        return 0;

    /* record the inverse of each insertion in the order the insertions happen.
       entry_apply replays the group in reverse, so undo removes the last
       inserted element first and unwinds the paste in the correct order. */
    UndoOp *ops = NULL;
    int num_ops = 0;
    int cap = 0;

    int r = row;
    int c = col;

    for (int i = 0; i < cb->len; i++)
    {
        char ch = cb->text[i];
        UndoOp op;

        if (ch == '\n')
        {
            /* split the current line at the cursor: the inverse merges the new
               line back into this one */
            buffer_insert_newline(buf, r, c);
            op.type = UNDO_OP_MERGE_LINE;
            op.row = r + 1;
            op.col = 0;
            op.ch = 0;
            r++;
            c = 0;
        }
        else
        {
            /* insert the character: the inverse deletes it again */
            buffer_insert_char(buf, r, c, ch);
            op.type = UNDO_OP_DELETE_CHAR;
            op.row = r;
            op.col = c;
            op.ch = ch;
            c++;
        }

        if (num_ops == cap)
        {
            cap = cap ? cap * 2 : 16;
            ops = realloc(ops, sizeof(UndoOp) * cap);
        }
        ops[num_ops++] = op;
    }

    /* the pre-edit cursor sits at the paste origin; the post-edit cursor sits
       at the end of the inserted text */
    history_record_group(hist, ops, num_ops, row, col, r, c);
    free(ops);

    *out_row = r;
    *out_col = c;
    return 1;
}
