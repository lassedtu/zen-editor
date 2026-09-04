#include "undo.h"

#include "buffer.h"

#include <stdlib.h>

/**
 * @file undo.c
 * @brief command-based undo/redo history implementation.
 *
 * each UndoEntry owns a heap-allocated array of operations that reverse a
 * performed edit. applying an operation against the buffer also yields its
 * inverse, so undoing an entry naturally produces the entry required to redo it
 * (and vice versa). this symmetry lets a single apply routine drive both
 * directions without storing forward and backward copies of every edit.
 *
 * typed characters coalesce: history_record_char_insert appends to the top
 * entry's ops array while the run continues, so one undo removes a whole word.
 */

/* ------------------------------------------------------------------ */
/* entry ownership                                                    */
/* ------------------------------------------------------------------ */

/**
 * @brief release the ops array owned by an entry.
 * @param entry pointer to the entry to free.
 */
static void entry_free(UndoEntry *entry)
{
    free(entry->ops);
    entry->ops = NULL;
    entry->num_ops = 0;
    entry->cap = 0;
}

/**
 * @brief append an operation to an entry, growing its ops array as needed.
 * @param entry pointer to the entry to append to.
 * @param op the operation to append.
 */
static void entry_push_op(UndoEntry *entry, UndoOp op)
{
    if (entry->num_ops == entry->cap)
    {
        int new_cap = entry->cap ? entry->cap * 2 : 8;
        entry->ops = realloc(entry->ops, sizeof(UndoOp) * new_cap);
        entry->cap = new_cap;
    }
    entry->ops[entry->num_ops++] = op;
}

/* ------------------------------------------------------------------ */
/* stack management                                                   */
/* ------------------------------------------------------------------ */

/**
 * @brief append an entry to a stack, growing it as needed.
 *
 * the stack takes ownership of the entry's ops array; the caller must not free
 * or reuse it after the push.
 *
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
 * @brief drop the oldest entry (index 0) from a stack, freeing its ops.
 * @param stack pointer to the stack to trim.
 */
static void stack_drop_oldest(UndoStack *stack)
{
    if (stack->count == 0)
        return;

    entry_free(&stack->entries[0]);
    for (int i = 1; i < stack->count; i++)
    {
        stack->entries[i - 1] = stack->entries[i];
    }
    stack->count--;
}

/**
 * @brief free every entry's ops and remove all entries from a stack.
 * @param stack pointer to the stack to clear.
 */
static void stack_clear(UndoStack *stack)
{
    for (int i = 0; i < stack->count; i++)
    {
        entry_free(&stack->entries[i]);
    }
    stack->count = 0;
}

/**
 * @brief free a stack's entries, their ops, and its backing array.
 * @param stack pointer to the stack to free.
 */
static void stack_free(UndoStack *stack)
{
    stack_clear(stack);
    free(stack->entries);
    stack->entries = NULL;
    stack->cap = 0;
}

/* ------------------------------------------------------------------ */
/* history lifecycle                                                  */
/* ------------------------------------------------------------------ */

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

/**
 * @brief end any open character-insert run so the next char starts fresh.
 * @param h pointer to the History.
 */
static void history_end_run(History *h)
{
    if (h->undo.count > 0)
    {
        h->undo.entries[h->undo.count - 1].is_char_run = 0;
    }
}

/**
 * @brief push a fully-formed entry, clearing redo and enforcing the cap.
 * @param h pointer to the History.
 * @param entry the entry to push (stack takes ownership of its ops).
 */
static void history_push_entry(History *h, UndoEntry entry)
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

void history_record(History *h, UndoOp op, int cursor_row, int cursor_col,
                    int redo_row, int redo_col)
{
    /* a discrete edit ends any coalescing run */
    history_end_run(h);

    UndoEntry entry;
    entry.ops = NULL;
    entry.num_ops = 0;
    entry.cap = 0;
    entry.is_char_run = 0;
    entry.cursor_row = cursor_row;
    entry.cursor_col = cursor_col;
    entry.redo_row = redo_row;
    entry.redo_col = redo_col;
    entry_push_op(&entry, op);

    history_push_entry(h, entry);
}

/**
 * @brief decide whether a typed character can extend the current run.
 *
 * a character coalesces into the top entry when that entry is an open character
 * run, the insertion is contiguous with the last op in the run (same row, next
 * column), and it does not begin a new word. a new word begins when a
 * non-whitespace character follows whitespace, so a word and its trailing
 * spaces undo as one step while the next word starts a fresh entry.
 *
 * @param h pointer to the History.
 * @param row row where the character is being inserted.
 * @param col column where the character is being inserted.
 * @param ch the character being inserted.
 * @return the top entry if the character should coalesce, otherwise NULL.
 */
static UndoEntry *coalesce_target(History *h, int row, int col, char ch)
{
    if (h->undo.count == 0)
        return NULL;

    UndoEntry *top = &h->undo.entries[h->undo.count - 1];
    if (!top->is_char_run || top->num_ops == 0)
        return NULL;

    UndoOp *last = &top->ops[top->num_ops - 1];
    /* the run stores inverse delete ops; the most recent character sits at the
       highest column, so the next contiguous insert lands at last->col + 1 */
    if (last->row != row || last->col + 1 != col)
        return NULL;

    /* a new word begins when a non-space follows whitespace: break there so the
       previous word (with its trailing spaces) is its own undo step */
    int last_is_space = (last->ch == ' ' || last->ch == '\t');
    int new_is_space = (ch == ' ' || ch == '\t');
    if (last_is_space && !new_is_space)
        return NULL;

    return top;
}

void history_record_char_insert(History *h, int row, int col, char ch)
{
    UndoEntry *target = coalesce_target(h, row, col, ch);

    if (target)
    {
        /* extend the existing run: append the inverse delete for this char and
           advance the post-edit (redo) cursor to just after it */
        UndoOp op = {UNDO_OP_DELETE_CHAR, row, col, ch};
        entry_push_op(target, op);
        target->redo_row = row;
        target->redo_col = col + 1;
        stack_clear(&h->redo);
        return;
    }

    /* start a new coalescing run */
    UndoEntry entry;
    entry.ops = NULL;
    entry.num_ops = 0;
    entry.cap = 0;
    entry.is_char_run = 1;
    entry.cursor_row = row;
    entry.cursor_col = col;
    entry.redo_row = row;
    entry.redo_col = col + 1;

    UndoOp op = {UNDO_OP_DELETE_CHAR, row, col, ch};
    entry_push_op(&entry, op);

    history_push_entry(h, entry);
}

/* ------------------------------------------------------------------ */
/* applying entries                                                   */
/* ------------------------------------------------------------------ */

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
 * single atomic step, and the collected inverses are stored (also owned on the
 * heap) so the entry can be replayed in the opposite direction. the two cursor
 * positions are swapped so that applying the inverse restores the opposite end.
 *
 * @param entry the entry whose operations should be applied.
 * @param buf pointer to the Buffer to modify.
 * @return the inverse entry, which owns its own ops array.
 */
static UndoEntry entry_apply(UndoEntry entry, Buffer *buf)
{
    UndoEntry inverse;
    inverse.ops = malloc(sizeof(UndoOp) * entry.num_ops);
    inverse.num_ops = entry.num_ops;
    inverse.cap = entry.num_ops;
    inverse.is_char_run = 0; /* replayed entries never continue coalescing */
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

    entry_free(&entry);
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

    entry_free(&entry);
    return 1;
}
