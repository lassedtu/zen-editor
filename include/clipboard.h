#ifndef ZE_CLIPBOARD_H
#define ZE_CLIPBOARD_H

#include "buffer.h"
#include "undo.h"

/**
 * @file clipboard.h
 * @brief internal clipboard buffer and paste operation.
 *
 * the clipboard holds the last copied or cut text. the editor core owns this
 * buffer, so copy, cut, and paste work without a system clipboard. the text
 * can contain newline characters, because a selection can span more than one
 * line. a later platform hook can connect this internal buffer to a system
 * clipboard, but the core does not need it.
 *
 * copy and cut store text into the clipboard. this module supplies the store
 * and the paste. the paste inserts the stored text into a buffer and records
 * one grouped entry in the undo history, so one undo reverses the whole paste.
 */

/**
 * @struct Clipboard
 * @brief holds the copied or cut text.
 */
typedef struct
{
    char *text; // heap-allocated clipboard text (may contain newlines)
    int len;    // length of the text, without the null terminator
} Clipboard;

/**
 * @brief initialize a clipboard to empty.
 * @param cb pointer to the Clipboard to initialize.
 */
void clipboard_init(Clipboard *cb);

/**
 * @brief free the text owned by a clipboard and reset it to empty.
 * @param cb pointer to the Clipboard to free.
 */
void clipboard_free(Clipboard *cb);

/**
 * @brief test whether the clipboard holds any text.
 * @param cb pointer to the Clipboard to test.
 * @return 1 if the clipboard is empty, 0 if it holds text.
 */
int clipboard_is_empty(const Clipboard *cb);

/**
 * @brief store a copy of the given text in the clipboard.
 *
 * frees any previous text and stores a private copy of the supplied text. the
 * clipboard owns the copy. a NULL or zero-length input clears the clipboard.
 *
 * @param cb pointer to the Clipboard to fill.
 * @param text pointer to the text to store.
 * @param len the length of the text, without the null terminator.
 */
void clipboard_set(Clipboard *cb, const char *text, int len);

/**
 * @brief insert the clipboard text into the buffer at the given position.
 *
 * splits the current line at each newline character in the clipboard text. the
 * cursor moves to the end of the inserted text. records one grouped entry in
 * the undo history, so one undo reverses the whole paste. does nothing when
 * the clipboard is empty.
 *
 * @param cb pointer to the Clipboard to read.
 * @param buf pointer to the Buffer to modify.
 * @param hist pointer to the History that records the grouped undo entry.
 * @param row the row where the text is inserted.
 * @param col the column where the text is inserted.
 * @param out_row set to the row where the cursor must move after the paste.
 * @param out_col set to the column where the cursor must move after the paste.
 * @return 1 if text was pasted, 0 if the clipboard was empty.
 */
int clipboard_paste(const Clipboard *cb, Buffer *buf, History *hist,
                    int row, int col, int *out_row, int *out_col);

#endif /* ZE_CLIPBOARD_H */
