/**
 * @file test_buffer.c
 * @brief unit tests for the buffer module.
 */

#include "buffer.h"
#include <stdlib.h>

TEST(buffer_create_has_one_empty_line)
{
    Buffer *buf = buffer_create();
    ASSERT(buf != NULL);
    ASSERT_EQ(buf->num_lines, 1);
    ASSERT_EQ(buf->lines[0].len, 0);
    ASSERT_EQ(buf->modified, 0);
    buffer_free(buf);
}

TEST(buffer_insert_char_at_start)
{
    Buffer *buf = buffer_create();
    buffer_insert_char(buf, 0, 0, 'b');
    buffer_insert_char(buf, 0, 0, 'a');
    ASSERT_EQ(buf->lines[0].len, 2);
    ASSERT(buf->lines[0].chars[0] == 'a');
    ASSERT(buf->lines[0].chars[1] == 'b');
    buffer_free(buf);
}

TEST(buffer_insert_char_at_end)
{
    Buffer *buf = buffer_create();
    buffer_insert_char(buf, 0, 0, 'a');
    buffer_insert_char(buf, 0, 1, 'b');
    buffer_insert_char(buf, 0, 2, 'c');
    ASSERT_EQ(buf->lines[0].len, 3);
    ASSERT(buf->lines[0].chars[0] == 'a');
    ASSERT(buf->lines[0].chars[1] == 'b');
    ASSERT(buf->lines[0].chars[2] == 'c');
    buffer_free(buf);
}

TEST(buffer_insert_char_at_middle)
{
    Buffer *buf = buffer_create();
    buffer_insert_char(buf, 0, 0, 'a');
    buffer_insert_char(buf, 0, 1, 'c');
    buffer_insert_char(buf, 0, 1, 'b');
    ASSERT_EQ(buf->lines[0].len, 3);
    ASSERT(buf->lines[0].chars[0] == 'a');
    ASSERT(buf->lines[0].chars[1] == 'b');
    ASSERT(buf->lines[0].chars[2] == 'c');
    buffer_free(buf);
}

TEST(buffer_insert_char_invalid_position)
{
    /* out-of-bounds row or col should be a no-op */
    Buffer *buf = buffer_create();
    buffer_insert_char(buf, 0, 0, 'a');
    buffer_insert_char(buf, -1, 0, 'x');
    buffer_insert_char(buf, 5, 0, 'x');
    buffer_insert_char(buf, 0, 99, 'x');
    ASSERT_EQ(buf->lines[0].len, 1);
    ASSERT(buf->lines[0].chars[0] == 'a');
    buffer_free(buf);
}

TEST(buffer_insert_char_grows_capacity)
{
    /* insert many characters to exercise growth — should not crash */
    Buffer *buf = buffer_create();
    for (int i = 0; i < 100; i++)
    {
        buffer_insert_char(buf, 0, i, 'x');
    }
    ASSERT_EQ(buf->lines[0].len, 100);
    buffer_free(buf);
}

TEST(buffer_delete_char_at_start)
{
    Buffer *buf = buffer_create();
    buffer_insert_char(buf, 0, 0, 'a');
    buffer_insert_char(buf, 0, 1, 'b');
    buffer_insert_char(buf, 0, 2, 'c');
    buffer_delete_char(buf, 0, 0);
    ASSERT_EQ(buf->lines[0].len, 2);
    ASSERT(buf->lines[0].chars[0] == 'b');
    ASSERT(buf->lines[0].chars[1] == 'c');
    buffer_free(buf);
}

TEST(buffer_delete_char_at_end_is_noop)
{
    /* deleting at col == len is out of bounds — no character there */
    Buffer *buf = buffer_create();
    buffer_insert_char(buf, 0, 0, 'a');
    buffer_delete_char(buf, 0, 1);
    ASSERT_EQ(buf->lines[0].len, 1);
    ASSERT(buf->lines[0].chars[0] == 'a');
    buffer_free(buf);
}

TEST(buffer_delete_char_at_middle)
{
    Buffer *buf = buffer_create();
    buffer_insert_char(buf, 0, 0, 'a');
    buffer_insert_char(buf, 0, 1, 'b');
    buffer_insert_char(buf, 0, 2, 'c');
    buffer_delete_char(buf, 0, 1);
    ASSERT_EQ(buf->lines[0].len, 2);
    ASSERT(buf->lines[0].chars[0] == 'a');
    ASSERT(buf->lines[0].chars[1] == 'c');
    buffer_free(buf);
}

TEST(buffer_delete_char_invalid_position)
{
    /* out-of-bounds row or col should be a no-op */
    Buffer *buf = buffer_create();
    buffer_insert_char(buf, 0, 0, 'a');
    buffer_delete_char(buf, -1, 0);
    buffer_delete_char(buf, 5, 0);
    buffer_delete_char(buf, 0, -1);
    buffer_delete_char(buf, 0, 99);
    ASSERT_EQ(buf->lines[0].len, 1);
    buffer_free(buf);
}

TEST(buffer_insert_newline_splits_line)
{
    /* "abcd" split at col 2 -> "ab" and "cd" */
    Buffer *buf = buffer_create();
    buffer_insert_char(buf, 0, 0, 'a');
    buffer_insert_char(buf, 0, 1, 'b');
    buffer_insert_char(buf, 0, 2, 'c');
    buffer_insert_char(buf, 0, 3, 'd');
    buffer_insert_newline(buf, 0, 2);
    ASSERT_EQ(buf->num_lines, 2);
    ASSERT_EQ(buf->lines[0].len, 2);
    ASSERT(buf->lines[0].chars[0] == 'a');
    ASSERT(buf->lines[0].chars[1] == 'b');
    ASSERT_EQ(buf->lines[1].len, 2);
    ASSERT(buf->lines[1].chars[0] == 'c');
    ASSERT(buf->lines[1].chars[1] == 'd');
    buffer_free(buf);
}

TEST(buffer_insert_newline_at_start)
{
    /* split at col 0 — original becomes empty, new line gets all content */
    Buffer *buf = buffer_create();
    buffer_insert_char(buf, 0, 0, 'a');
    buffer_insert_char(buf, 0, 1, 'b');
    buffer_insert_newline(buf, 0, 0);
    ASSERT_EQ(buf->num_lines, 2);
    ASSERT_EQ(buf->lines[0].len, 0);
    ASSERT_EQ(buf->lines[1].len, 2);
    ASSERT(buf->lines[1].chars[0] == 'a');
    ASSERT(buf->lines[1].chars[1] == 'b');
    buffer_free(buf);
}

TEST(buffer_insert_newline_at_end)
{
    /* split at end — original keeps content, new line is empty */
    Buffer *buf = buffer_create();
    buffer_insert_char(buf, 0, 0, 'a');
    buffer_insert_char(buf, 0, 1, 'b');
    buffer_insert_newline(buf, 0, 2);
    ASSERT_EQ(buf->num_lines, 2);
    ASSERT_EQ(buf->lines[0].len, 2);
    ASSERT(buf->lines[0].chars[0] == 'a');
    ASSERT(buf->lines[0].chars[1] == 'b');
    ASSERT_EQ(buf->lines[1].len, 0);
    buffer_free(buf);
}

TEST(buffer_delete_line_merges_with_previous)
{
    /* "ab" + "cd" merged -> "abcd" */
    Buffer *buf = buffer_create();
    buffer_insert_char(buf, 0, 0, 'a');
    buffer_insert_char(buf, 0, 1, 'b');
    buffer_insert_newline(buf, 0, 2);
    buffer_insert_char(buf, 1, 0, 'c');
    buffer_insert_char(buf, 1, 1, 'd');
    buffer_delete_line(buf, 1);
    ASSERT_EQ(buf->num_lines, 1);
    ASSERT_EQ(buf->lines[0].len, 4);
    ASSERT(buf->lines[0].chars[0] == 'a');
    ASSERT(buf->lines[0].chars[1] == 'b');
    ASSERT(buf->lines[0].chars[2] == 'c');
    ASSERT(buf->lines[0].chars[3] == 'd');
    buffer_free(buf);
}

TEST(buffer_delete_line_first_row_is_noop)
{
    /* row 0 cannot be merged with a previous line */
    Buffer *buf = buffer_create();
    buffer_insert_char(buf, 0, 0, 'a');
    buffer_delete_line(buf, 0);
    ASSERT_EQ(buf->num_lines, 1);
    ASSERT_EQ(buf->lines[0].len, 1);
    buffer_free(buf);
}

TEST(buffer_modified_flag)
{
    Buffer *buf = buffer_create();
    ASSERT_EQ(buf->modified, 0);
    buffer_insert_char(buf, 0, 0, 'a');
    ASSERT_EQ(buf->modified, 1);
    buffer_free(buf);
}
