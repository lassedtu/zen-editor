/**
 * @file test_cursor.c
 * @brief unit tests for the cursor module.
 */

#include "cursor.h"
#include <stdlib.h>

TEST(cursor_move_up_from_top_is_noop)
{
    Buffer *buf = buffer_create();
    Cursor cur = {0, 0};
    cursor_move_up(&cur, buf);
    ASSERT_EQ(cur.row, 0);
    ASSERT_EQ(cur.col, 0);
    buffer_free(buf);
}

TEST(cursor_move_up_clamps_col)
{
    /* line 0: "ab", line 1: "cdef" — moving up from col 3 should clamp to 2 */
    Buffer *buf = buffer_create();
    buffer_insert_char(buf, 0, 0, 'a');
    buffer_insert_char(buf, 0, 1, 'b');
    buffer_insert_newline(buf, 0, 2);
    buffer_insert_char(buf, 1, 0, 'c');
    buffer_insert_char(buf, 1, 1, 'd');
    buffer_insert_char(buf, 1, 2, 'e');
    buffer_insert_char(buf, 1, 3, 'f');

    Cursor cur = {1, 3};
    cursor_move_up(&cur, buf);
    ASSERT_EQ(cur.row, 0);
    ASSERT_EQ(cur.col, 2);
    buffer_free(buf);
}

TEST(cursor_move_down_from_bottom_is_noop)
{
    Buffer *buf = buffer_create();
    Cursor cur = {0, 0};
    cursor_move_down(&cur, buf);
    ASSERT_EQ(cur.row, 0);
    ASSERT_EQ(cur.col, 0);
    buffer_free(buf);
}

TEST(cursor_move_down_clamps_col)
{
    /* line 0: "abcd", line 1: "ef" — moving down from col 3 should clamp to 2 */
    Buffer *buf = buffer_create();
    buffer_insert_char(buf, 0, 0, 'a');
    buffer_insert_char(buf, 0, 1, 'b');
    buffer_insert_char(buf, 0, 2, 'c');
    buffer_insert_char(buf, 0, 3, 'd');
    buffer_insert_newline(buf, 0, 4);
    buffer_insert_char(buf, 1, 0, 'e');
    buffer_insert_char(buf, 1, 1, 'f');

    Cursor cur = {0, 3};
    cursor_move_down(&cur, buf);
    ASSERT_EQ(cur.row, 1);
    ASSERT_EQ(cur.col, 2);
    buffer_free(buf);
}

TEST(cursor_move_left_wraps_to_previous_line)
{
    /* at col 0 of line 1, should wrap to end of line 0 */
    Buffer *buf = buffer_create();
    buffer_insert_char(buf, 0, 0, 'a');
    buffer_insert_char(buf, 0, 1, 'b');
    buffer_insert_newline(buf, 0, 2);

    Cursor cur = {1, 0};
    cursor_move_left(&cur, buf);
    ASSERT_EQ(cur.row, 0);
    ASSERT_EQ(cur.col, 2);
    buffer_free(buf);
}

TEST(cursor_move_left_at_origin_is_noop)
{
    Buffer *buf = buffer_create();
    Cursor cur = {0, 0};
    cursor_move_left(&cur, buf);
    ASSERT_EQ(cur.row, 0);
    ASSERT_EQ(cur.col, 0);
    buffer_free(buf);
}

TEST(cursor_move_right_wraps_to_next_line)
{
    /* at end of line 0, should wrap to col 0 of line 1 */
    Buffer *buf = buffer_create();
    buffer_insert_char(buf, 0, 0, 'a');
    buffer_insert_char(buf, 0, 1, 'b');
    buffer_insert_newline(buf, 0, 2);

    Cursor cur = {0, 2};
    cursor_move_right(&cur, buf);
    ASSERT_EQ(cur.row, 1);
    ASSERT_EQ(cur.col, 0);
    buffer_free(buf);
}

TEST(cursor_move_right_at_end_is_noop)
{
    /* at end of last line, nowhere to go */
    Buffer *buf = buffer_create();
    buffer_insert_char(buf, 0, 0, 'a');
    buffer_insert_char(buf, 0, 1, 'b');

    Cursor cur = {0, 2};
    cursor_move_right(&cur, buf);
    ASSERT_EQ(cur.row, 0);
    ASSERT_EQ(cur.col, 2);
    buffer_free(buf);
}

TEST(cursor_home_moves_to_col_zero)
{
    Buffer *buf = buffer_create();
    buffer_insert_char(buf, 0, 0, 'a');
    buffer_insert_char(buf, 0, 1, 'b');
    buffer_insert_char(buf, 0, 2, 'c');

    Cursor cur = {0, 3};
    cursor_home(&cur);
    ASSERT_EQ(cur.col, 0);
    buffer_free(buf);
}

TEST(cursor_end_moves_to_line_length)
{
    Buffer *buf = buffer_create();
    buffer_insert_char(buf, 0, 0, 'a');
    buffer_insert_char(buf, 0, 1, 'b');
    buffer_insert_char(buf, 0, 2, 'c');

    Cursor cur = {0, 0};
    cursor_end(&cur, buf);
    ASSERT_EQ(cur.col, 3);
    buffer_free(buf);
}

TEST(cursor_clamp_corrects_invalid_position)
{
    /* negative and beyond-bounds values get clamped to valid range */
    Buffer *buf = buffer_create();
    buffer_insert_char(buf, 0, 0, 'a');
    buffer_insert_char(buf, 0, 1, 'b');

    Cursor cur = {-5, -3};
    cursor_clamp(&cur, buf);
    ASSERT_EQ(cur.row, 0);
    ASSERT_EQ(cur.col, 0);

    cur.row = 99;
    cur.col = 99;
    cursor_clamp(&cur, buf);
    ASSERT_EQ(cur.row, 0);
    ASSERT_EQ(cur.col, 2);

    buffer_free(buf);
}
