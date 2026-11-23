#ifndef ZE_CURSOR_H
#define ZE_CURSOR_H

#include "buffer.h"

/**
 * @file cursor.h
 * @brief cursor data structure for text editing.
 *
 * This file defines the Cursor structure, which represents the position of the cursor in the text buffer, along with functions to manipulate it.
 */

/**
 * @struct Cursor
 * @brief represents the position of the cursor in the text buffer.
 */
typedef struct
{
    int row; // row index of the cursor in the buffer
    int col; // column index of the cursor in the buffer
} Cursor;

/**
 * @brief move the cursor in the specified direction within the buffer.
 * @param cur pointer to the Cursor to be moved.
 * @param buf pointer to the Buffer in which the cursor is moving.
 */
void cursor_move_up(Cursor *cur, Buffer *buf);
void cursor_move_down(Cursor *cur, Buffer *buf);
void cursor_move_left(Cursor *cur, Buffer *buf);
void cursor_move_right(Cursor *cur, Buffer *buf);

/**
 * @brief move the cursor left to the start of the previous word.
 *
 * skips any whitespace to the left of the cursor, then skips the run of
 * characters that has the same class (word characters or punctuation). the
 * cursor stops at the start of that run. the move wraps to the end of the
 * previous line when the cursor is at the start of a line.
 *
 * @param cur pointer to the Cursor to be moved.
 * @param buf pointer to the Buffer in which the cursor is moving.
 */
void cursor_move_word_left(Cursor *cur, Buffer *buf);

/**
 * @brief move the cursor right to the end of the next word.
 *
 * skips any whitespace to the right of the cursor, then skips the run of
 * characters that has the same class (word characters or punctuation). the
 * cursor stops after the end of that run. the move wraps to the start of the
 * next line when the cursor is at the end of a line.
 *
 * @param cur pointer to the Cursor to be moved.
 * @param buf pointer to the Buffer in which the cursor is moving.
 */
void cursor_move_word_right(Cursor *cur, Buffer *buf);

/**
 * @brief move the cursor to the beginning of the current line.
 * @param cur pointer to the Cursor to be moved.
 */
void cursor_home(Cursor *cur);

/**
 * @brief move the cursor to the end of the current line.
 * @param cur pointer to the Cursor to be moved.
 * @param buf pointer to the Buffer in which the cursor is moving.
 */
void cursor_end(Cursor *cur, Buffer *buf);

/**
 * @brief clamp the cursor position to ensure it remains within the bounds of the buffer.
 * @param cur pointer to the Cursor to be clamped.
 * @param buf pointer to the Buffer in which the cursor is clamped.
 */
void cursor_clamp(Cursor *cur, Buffer *buf);

#endif /* ZE_CURSOR_H */
