#ifndef ZE_SELECTION_H
#define ZE_SELECTION_H

#include "buffer.h"
#include "undo.h"

/**
 * @file selection.h
 * @brief text selection state and region operations.
 *
 * a selection marks a region of text between two points: the anchor and the
 * cursor. the anchor is the point where the selection starts. the cursor is
 * the point where the selection ends. the user can move the cursor point in
 * any direction, so the anchor can come before or after the cursor in the
 * buffer. callers must normalize the two points into reading order before
 * they read or delete the region.
 *
 * the selection lives in the editor core. it has no dependency on the platform
 * layer, so it is fully unit-testable. region deletion records one grouped
 * entry in the undo history, so one undo restores the whole region.
 */

/**
 * @struct Selection
 * @brief holds the state of an active or inactive text selection.
 */
typedef struct
{
    int active;     // 1 while a selection is active, 0 when there is none
    int anchor_row; // row of the fixed anchor point
    int anchor_col; // column of the fixed anchor point
    int cursor_row; // row of the moving cursor point
    int cursor_col; // column of the moving cursor point
} Selection;

/**
 * @brief clear a selection so that no region is marked.
 * @param sel pointer to the Selection to clear.
 */
void selection_clear(Selection *sel);

/**
 * @brief start a selection at the given point.
 *
 * sets the anchor and the cursor point to the same position and marks the
 * selection active. the caller extends the selection later with
 * selection_set_cursor.
 *
 * @param sel pointer to the Selection to start.
 * @param row the row of the start point.
 * @param col the column of the start point.
 */
void selection_start(Selection *sel, int row, int col);

/**
 * @brief move the cursor point of an active selection.
 *
 * keeps the anchor fixed and moves the cursor point to the new position. does
 * nothing when the selection is not active.
 *
 * @param sel pointer to the Selection to extend.
 * @param row the new row of the cursor point.
 * @param col the new column of the cursor point.
 */
void selection_set_cursor(Selection *sel, int row, int col);

/**
 * @brief return the region in reading order.
 *
 * writes the start point and the end point of the region into the output
 * parameters. the start point is the point that comes first in the buffer.
 * the end point is the point that comes last. the caller uses these points to
 * read or delete the region.
 *
 * @param sel pointer to the Selection to normalize.
 * @param start_row set to the row of the start point.
 * @param start_col set to the column of the start point.
 * @param end_row set to the row of the end point.
 * @param end_col set to the column of the end point.
 */
void selection_normalize(const Selection *sel, int *start_row, int *start_col,
                         int *end_row, int *end_col);

/**
 * @brief test whether a buffer position is inside the selected region.
 *
 * the start point is inside the region. the end point is not, so the region is
 * a half-open range. the renderer uses this function to decide which
 * characters to draw with inverted video.
 *
 * @param sel pointer to the Selection to test.
 * @param row the row of the position to test.
 * @param col the column of the position to test.
 * @return 1 if the position is inside the region, 0 if it is not or the
 *         selection is inactive.
 */
int selection_contains(const Selection *sel, int row, int col);

/**
 * @brief copy the selected region text into a heap buffer.
 *
 * returns the text of the normalized region. line breaks in the region become
 * newline characters. the caller owns the returned buffer and must free it.
 * writes the length of the text, without the null terminator, into out_len.
 *
 * @param sel pointer to the Selection to read.
 * @param buf pointer to the Buffer to read from.
 * @param out_len set to the length of the returned text.
 * @return a null-terminated heap buffer with the region text, or NULL when the
 *         selection is inactive or empty.
 */
char *selection_copy_region(const Selection *sel, const Buffer *buf,
                            int *out_len);

/**
 * @brief delete the selected region from the buffer.
 *
 * removes all text in the normalized region and joins the start line with the
 * end line. records one grouped entry in the undo history, so one undo
 * restores the full region. writes the start point of the region into
 * out_row/out_col, which is where the cursor must move after the deletion.
 *
 * @param sel pointer to the Selection to delete.
 * @param buf pointer to the Buffer to modify.
 * @param hist pointer to the History that records the grouped undo entry.
 * @param out_row set to the row where the cursor must move.
 * @param out_col set to the column where the cursor must move.
 * @return 1 if a region was deleted, 0 if the selection was inactive or empty.
 */
int selection_delete_region(const Selection *sel, Buffer *buf, History *hist,
                            int *out_row, int *out_col);

#endif /* ZE_SELECTION_H */
