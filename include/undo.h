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
 * @def UNDO_MAX_OPS
 * @brief maximum low-level operations grouped into a single logical edit.
 */
#define UNDO_MAX_OPS 4

/**
 * @struct UndoEntry
 * @brief a single logical edit, grouping one or more low-level operations.
 *
 * operations are applied in reverse index order when applied so that grouped
 * edits (e.g. a line merge that is a delete-line plus an implied append) are
 * reversed as one atomic step. two cursor positions are captured: where the
 * cursor should sit after this entry's operations are applied, and where it sat
 * before. this lets undo restore the pre-edit position while redo restores the
 * post-edit position.
 */
typedef struct
{
    UndoOp ops[UNDO_MAX_OPS]; // grouped operations
    int num_ops;              // number of operations in this entry
    int cursor_row;           // cursor row to restore after applying this entry
    int cursor_col;           // cursor col to restore after applying this entry
    int redo_row;             // cursor row to restore after the inverse is applied
    int redo_col;             // cursor col to restore after the inverse is applied
} UndoEntry;

/**
 * @struct UndoStack
 * @brief a dynamically-sized, capped stack of undo entries.
 */
typedef struct
{
    UndoEntry *entries; // dynamic array of entries
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
 * @brief record a completed edit as a new undo entry.
 *
 * pushes the entry onto the undo stack and clears the redo stack (standard
 * undo/redo semantics: a fresh edit invalidates the redo history). when the
 * undo stack exceeds UNDO_MAX_ENTRIES the oldest entry is dropped.
 *
 * @param h pointer to the History.
 * @param entry the entry describing how to reverse the edit.
 */
void history_record(History *h, UndoEntry entry);

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
