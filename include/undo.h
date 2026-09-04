#ifndef ZE_UNDO_H
#define ZE_UNDO_H

#include "buffer.h"

/**
 * @file undo.h
 * @brief command-based undo/redo history for the editor.
 *
 * every reversible edit is recorded as one or more low-level operations grouped
 * into a single UndoEntry. undoing an entry applies the inverse of each of its
 * operations in reverse order, which restores the buffer to its prior state and
 * pushes the entry onto the redo stack. redoing re-applies the original edit.
 *
 * consecutive typed characters coalesce into a single entry so one undo step
 * removes a whole word rather than one character at a time (see
 * history_record_char_insert). because a coalesced run can be arbitrarily long,
 * each entry owns a dynamically-sized array of operations.
 *
 * the history operates purely on a Buffer plus a saved cursor position, so it
 * has no dependency on the platform layer and is fully unit-testable.
 */

/**
 * @def UNDO_MAX_ENTRIES
 * @brief maximum number of entries retained per stack before the oldest is dropped.
 */
#define UNDO_MAX_ENTRIES 1000

/**
 * @enum UndoOpType
 * @brief identifies a single low-level reversible operation.
 *
 * each op is stored as the action required to *undo* a performed edit. for
 * example, inserting a character is recorded as an UNDO_OP_DELETE_CHAR that
 * removes it again.
 */
typedef enum
{
    UNDO_OP_INSERT_CHAR,  // reinsert a character at (row, col)
    UNDO_OP_DELETE_CHAR,  // delete the character at (row, col)
    UNDO_OP_SPLIT_LINE,   // split the line at (row, col) (inverse of a merge)
    UNDO_OP_MERGE_LINE,   // merge line `row` into the previous line
} UndoOpType;

/**
 * @struct UndoOp
 * @brief a single low-level operation used to reverse (or replay) an edit.
 */
typedef struct
{
    UndoOpType type; // which operation to apply
    int row;         // row the operation targets
    int col;         // column the operation targets
    char ch;         // character payload (UNDO_OP_INSERT_CHAR only)
} UndoOp;

/**
 * @struct UndoEntry
 * @brief a single logical edit, grouping one or more low-level operations.
 *
 * operations are applied in reverse index order so that grouped edits (a line
 * merge, or a coalesced run of typed characters) are reversed as one atomic
 * step. the ops array is heap-allocated and owned by the entry. two cursor
 * positions are captured: where the cursor should sit after this entry's
 * operations are applied, and where it sat before, so undo restores the
 * pre-edit position while redo restores the post-edit position.
 */
typedef struct
{
    UndoOp *ops;    // heap-allocated array of grouped operations
    int num_ops;    // number of operations in this entry
    int cap;        // allocated capacity of the ops array
    int is_char_run; // 1 if this entry is a coalescing run of typed characters
    int cursor_row; // cursor row to restore after applying this entry
    int cursor_col; // cursor col to restore after applying this entry
    int redo_row;   // cursor row to restore after the inverse is applied
    int redo_col;   // cursor col to restore after the inverse is applied
} UndoEntry;

/**
 * @struct UndoStack
 * @brief a dynamically-sized, capped stack of undo entries.
 */
typedef struct
{
    UndoEntry *entries; // dynamic array of entries (each owns its ops array)
    int count;          // number of entries currently on the stack
    int cap;            // allocated capacity of the entries array
} UndoStack;

/**
 * @struct History
 * @brief holds the undo and redo stacks for an editor session.
 */
typedef struct
{
    UndoStack undo; // edits that can be undone (most recent on top)
    UndoStack redo; // undone edits that can be reapplied
} History;

/**
 * @brief initialize a history with empty undo and redo stacks.
 * @param h pointer to the History to initialize.
 */
void history_init(History *h);

/**
 * @brief free all memory owned by a history and reset it to empty.
 * @param h pointer to the History to free.
 */
void history_free(History *h);

/**
 * @brief record a single non-coalescing edit as a new undo entry.
 *
 * pushes a fresh entry holding the one inverse operation and clears the redo
 * stack. use this for edits that should each be their own undo step (delete,
 * backspace, newline). any open character-insert run is ended so the next
 * typed character starts a new run.
 *
 * @param h pointer to the History.
 * @param op the inverse operation that reverses the edit.
 * @param cursor_row cursor row to restore when the edit is undone (pre-edit).
 * @param cursor_col cursor col to restore when the edit is undone (pre-edit).
 * @param redo_row cursor row to restore when the edit is redone (post-edit).
 * @param redo_col cursor col to restore when the edit is redone (post-edit).
 */
void history_record(History *h, UndoOp op, int cursor_row, int cursor_col,
                    int redo_row, int redo_col);

/**
 * @brief record a typed character, coalescing it with the previous run.
 *
 * consecutive characters typed contiguously merge into the top undo entry so a
 * single undo removes a whole word. a new entry is started when there is no
 * open run, when the insertion is not contiguous with the run, or when a word
 * boundary is crossed (a run is closed after whitespace so the next word begins
 * a fresh entry). the redo stack is cleared on every recorded character.
 *
 * @param h pointer to the History.
 * @param row row where the character was inserted.
 * @param col column where the character was inserted.
 * @param ch the character that was inserted.
 */
void history_record_char_insert(History *h, int row, int col, char ch);

/**
 * @brief undo the most recent edit.
 *
 * pops the top undo entry, applies its inverse operations to the buffer, pushes
 * a redo entry, and writes the restored cursor position into out_row/out_col.
 *
 * @param h pointer to the History.
 * @param buf pointer to the Buffer to modify.
 * @param out_row set to the cursor row to restore (unchanged if nothing to undo).
 * @param out_col set to the cursor col to restore (unchanged if nothing to undo).
 * @return 1 if an edit was undone, 0 if the undo stack was empty.
 */
int history_undo(History *h, Buffer *buf, int *out_row, int *out_col);

/**
 * @brief redo the most recently undone edit.
 *
 * pops the top redo entry, re-applies the original edit to the buffer, pushes
 * the entry back onto the undo stack, and writes the cursor position into
 * out_row/out_col.
 *
 * @param h pointer to the History.
 * @param buf pointer to the Buffer to modify.
 * @param out_row set to the cursor row to restore (unchanged if nothing to redo).
 * @param out_col set to the cursor col to restore (unchanged if nothing to redo).
 * @return 1 if an edit was redone, 0 if the redo stack was empty.
 */
int history_redo(History *h, Buffer *buf, int *out_row, int *out_col);

#endif /* ZE_UNDO_H */
