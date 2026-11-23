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

/**
 * @brief fill a buffer with text, splitting lines on newline characters.
 * @param buf pointer to the Buffer to fill.
 * @param s the null-terminated string to insert.
 */
static void cursor_fill_buffer(Buffer *buf, const char *s)
{
    int row = 0;
    int col = 0;
    for (const char *p = s; *p; p++)
    {
        if (*p == '\n')
        {
            buffer_insert_newline(buf, row, col);
            row++;
            col = 0;
        }
        else
        {
            buffer_insert_char(buf, row, col, *p);
            col++;
        }
    }
}

TEST(word_right_stops_at_end_of_word)
{
    /* "hello world" from col 0 -> end of "hello" at col 5 */
    Buffer *buf = buffer_create();
    cursor_fill_buffer(buf, "hello world");
    Cursor cur = {0, 0};
    cursor_move_word_right(&cur, buf);
    ASSERT_EQ(cur.row, 0);
    ASSERT_EQ(cur.col, 5);
    buffer_free(buf);
}

TEST(word_right_skips_leading_spaces)
{
    /* from col 5 (the space) -> skips space then "world" -> col 11 */
    Buffer *buf = buffer_create();
    cursor_fill_buffer(buf, "hello world");
    Cursor cur = {0, 5};
    cursor_move_word_right(&cur, buf);
    ASSERT_EQ(cur.col, 11);
    buffer_free(buf);
}

TEST(word_right_treats_punctuation_as_its_own_run)
{
    /* "foo.bar" from col 0 -> "foo" ends at col 3 (punctuation is separate) */
    Buffer *buf = buffer_create();
    cursor_fill_buffer(buf, "foo.bar");
    Cursor cur = {0, 0};
    cursor_move_word_right(&cur, buf);
    ASSERT_EQ(cur.col, 3);
    /* next move consumes the "." run -> col 4 */
    cursor_move_word_right(&cur, buf);
    ASSERT_EQ(cur.col, 4);
    /* next move consumes "bar" -> col 7 */
    cursor_move_word_right(&cur, buf);
    ASSERT_EQ(cur.col, 7);
    buffer_free(buf);
}

TEST(word_right_wraps_to_next_line_at_end)
{
    Buffer *buf = buffer_create();
    cursor_fill_buffer(buf, "ab\ncd");
    Cursor cur = {0, 2}; /* end of line 0 */
    cursor_move_word_right(&cur, buf);
    ASSERT_EQ(cur.row, 1);
    ASSERT_EQ(cur.col, 0);
    buffer_free(buf);
}

TEST(word_left_stops_at_start_of_word)
{
    /* "hello world" from col 11 -> start of "world" at col 6 */
    Buffer *buf = buffer_create();
    cursor_fill_buffer(buf, "hello world");
    Cursor cur = {0, 11};
    cursor_move_word_left(&cur, buf);
    ASSERT_EQ(cur.col, 6);
    /* again -> skip the space, then start of "hello" at col 0 */
    cursor_move_word_left(&cur, buf);
    ASSERT_EQ(cur.col, 0);
    buffer_free(buf);
}

TEST(word_left_skips_trailing_spaces)
{
    /* from col 6 (start of "world"): the char to the left is a space, so it
       skips the space and lands at the start of "hello" (col 0) */
    Buffer *buf = buffer_create();
    cursor_fill_buffer(buf, "hello world");
    Cursor cur = {0, 6};
    cursor_move_word_left(&cur, buf);
    ASSERT_EQ(cur.col, 0);
    buffer_free(buf);
}

TEST(word_left_treats_punctuation_as_its_own_run)
{
    /* "foo.bar" from col 7 -> start of "bar" at col 4 */
    Buffer *buf = buffer_create();
    cursor_fill_buffer(buf, "foo.bar");
    Cursor cur = {0, 7};
    cursor_move_word_left(&cur, buf);
    ASSERT_EQ(cur.col, 4);
    /* again -> the "." run -> col 3 */
    cursor_move_word_left(&cur, buf);
    ASSERT_EQ(cur.col, 3);
    /* again -> "foo" -> col 0 */
    cursor_move_word_left(&cur, buf);
    ASSERT_EQ(cur.col, 0);
    buffer_free(buf);
}

TEST(word_left_wraps_to_previous_line_at_start)
{
    Buffer *buf = buffer_create();
    cursor_fill_buffer(buf, "ab\ncd");
    Cursor cur = {1, 0}; /* start of line 1 */
    cursor_move_word_left(&cur, buf);
    ASSERT_EQ(cur.row, 0);
    ASSERT_EQ(cur.col, 2);
    buffer_free(buf);
}

TEST(word_left_at_origin_is_noop)
{
    Buffer *buf = buffer_create();
    cursor_fill_buffer(buf, "abc");
    Cursor cur = {0, 0};
    cursor_move_word_left(&cur, buf);
    ASSERT_EQ(cur.row, 0);
    ASSERT_EQ(cur.col, 0);
    buffer_free(buf);
}

TEST(word_right_at_end_of_buffer_is_noop)
{
    Buffer *buf = buffer_create();
    cursor_fill_buffer(buf, "abc");
    Cursor cur = {0, 3};
    cursor_move_word_right(&cur, buf);
    ASSERT_EQ(cur.row, 0);
    ASSERT_EQ(cur.col, 3);
    buffer_free(buf);
}
