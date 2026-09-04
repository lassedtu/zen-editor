#include "undo.h"

#include "buffer.h"

#include <stdlib.h>

/**
 * @file undo.c
 * @brief command-based undo/redo history implementation.
 *
 * each UndoEntry stores the operations needed to reverse a performed edit.
 * applying an operation against the buffer also yields its inverse, so undoing
 * an entry naturally produces the entry required to redo it (and vice versa).
 * this symmetry lets a single apply routine drive both directions without
 * storing forward and backward copies of every edit.
 */

/**
 * @brief append an entry to a stack, growing it as needed.
 * @param stack pointer to the stack to push onto.
 * @param entry the entry to append.
 */
static void stack_push(UndoStack *stack, UndoEntry entry)
{
    if (stack->count == stack->cap)
    {
        int new_cap = stack->cap ? stack->cap * 2 : 16;
        stack->entries = realloc(stack->entries, sizeof(UndoEntry) * new_cap);
        stack->cap = new_cap;
    }
    stack->entries[stack->count++] = entry;
}

/**
 * @brief drop the oldest entry (index 0) from a stack, shifting the rest down.
 * @param stack pointer to the stack to trim.
 */
static void stack_drop_oldest(UndoStack *stack)
{
    if (stack->count == 0)
        return;

    for (int i = 1; i < stack->count; i++)
    {
        stack->entries[i - 1] = stack->entries[i];
    }
    stack->count--;
}

/**
 * @brief remove all entries from a stack without freeing its backing array.
 * @param stack pointer to the stack to clear.
 */
static void stack_clear(UndoStack *stack)
{
    stack->count = 0;
}

/**
 * @brief free a stack's backing array and reset it to empty.
 * @param stack pointer to the stack to free.
 */
static void stack_free(UndoStack *stack)
{
    free(stack->entries);
    stack->entries = NULL;
    stack->count = 0;
    stack->cap = 0;
}

void history_init(History *h)
{
    h->undo.entries = NULL;
    h->undo.count = 0;
    h->undo.cap = 0;
    h->redo.entries = NULL;
    h->redo.count = 0;
    h->redo.cap = 0;
}

void history_free(History *h)
{
    stack_free(&h->undo);
    stack_free(&h->redo);
}

void history_record(History *h, UndoEntry entry)
{
    /* a fresh edit invalidates any redo history */
    stack_clear(&h->redo);

    stack_push(&h->undo, entry);

    /* enforce the history cap by dropping the oldest entry */
    if (h->undo.count > UNDO_MAX_ENTRIES)
    {
        stack_drop_oldest(&h->undo);
    }
}

/**
 * @brief apply a single operation to the buffer and return its inverse.
 *
 * the returned op, when applied, undoes the effect of the op just applied. this
 * is what lets undo and redo share one code path: undoing collects inverses to
 * form the redo entry, and redoing collects inverses to reform the undo entry.
 *
 * @param op the operation to apply.
 * @param buf pointer to the Buffer to modify.
 * @return the inverse operation.
 */
static UndoOp op_apply(UndoOp op, Buffer *buf)
{
    UndoOp inverse = op;

    switch (op.type)
    {
    case UNDO_OP_INSERT_CHAR:
        buffer_insert_char(buf, op.row, op.col, op.ch);
        inverse.type = UNDO_OP_DELETE_CHAR;
        break;

    case UNDO_OP_DELETE_CHAR:
        /* capture the character before deleting so the inverse can restore it */
        if (op.row >= 0 && op.row < buf->num_lines &&
            op.col >= 0 && op.col < buf->lines[op.row].len)
        {
            inverse.ch = buf->lines[op.row].chars[op.col];
        }
        buffer_delete_char(buf, op.row, op.col);
        inverse.type = UNDO_OP_INSERT_CHAR;
        break;

    case UNDO_OP_SPLIT_LINE:
        buffer_insert_newline(buf, op.row, op.col);
        inverse.type = UNDO_OP_MERGE_LINE;
        /* the merge that reverses this split targets the newly created line */
        inverse.row = op.row + 1;
        inverse.col = 0;
        break;

    case UNDO_OP_MERGE_LINE:
        /* the split that reverses this merge happens at the join column on the
           previous line, which is that line's length before the merge */
        if (op.row > 0 && op.row < buf->num_lines)
        {
            inverse.col = buf->lines[op.row - 1].len;
        }
        buffer_delete_line(buf, op.row);
        inverse.type = UNDO_OP_SPLIT_LINE;
        inverse.row = op.row - 1;
        break;
    }

    return inverse;
}

/**
 * @brief apply every operation in an entry and build the inverse entry.
 *
 * operations are applied in reverse index order so grouped edits reverse as a
 * single atomic step, and the collected inverses are stored so the entry can be
 * replayed in the opposite direction.
 *
 * @param entry the entry whose operations should be applied.
 * @param buf pointer to the Buffer to modify.
 * @return the inverse entry, preserving the cursor position.
 */
static UndoEntry entry_apply(UndoEntry entry, Buffer *buf)
{
    UndoEntry inverse;
    inverse.num_ops = entry.num_ops;
    /* the inverse entry restores the opposite cursor position: applying it must
       leave the cursor where it was before this entry was applied */
    inverse.cursor_row = entry.redo_row;
    inverse.cursor_col = entry.redo_col;
    inverse.redo_row = entry.cursor_row;
    inverse.redo_col = entry.cursor_col;

    /* apply in reverse; the inverse ops are stored so that applying the inverse
       entry (also in reverse) reproduces the original operation order */
    for (int i = entry.num_ops - 1; i >= 0; i--)
    {
        UndoOp inv = op_apply(entry.ops[i], buf);
        inverse.ops[entry.num_ops - 1 - i] = inv;
    }

    return inverse;
}

int history_undo(History *h, Buffer *buf, int *out_row, int *out_col)
{
    if (h->undo.count == 0)
        return 0;

    UndoEntry entry = h->undo.entries[--h->undo.count];
    UndoEntry redo_entry = entry_apply(entry, buf);

    stack_push(&h->redo, redo_entry);

    *out_row = entry.cursor_row;
    *out_col = entry.cursor_col;
    return 1;
}

int history_redo(History *h, Buffer *buf, int *out_row, int *out_col)
{
    if (h->redo.count == 0)
        return 0;

    UndoEntry entry = h->redo.entries[--h->redo.count];
    UndoEntry undo_entry = entry_apply(entry, buf);

    stack_push(&h->undo, undo_entry);

    *out_row = entry.cursor_row;
    *out_col = entry.cursor_col;
    return 1;
}
