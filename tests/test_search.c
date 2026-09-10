/**
 * @file test_search.c
 * @brief unit tests for the search module.
 */

#include "search.h"
#include "buffer.h"
#include <stdlib.h>

/**
 * @brief helper to fill a fresh buffer's first line with a string.
 * @param buf pointer to the buffer to write into.
 * @param s null-terminated string to insert at row 0.
 */
static void fill_line(Buffer *buf, const char *s)
{
    int col = 0;
    for (const char *p = s; *p != '\0'; p++)
    {
        buffer_insert_char(buf, 0, col++, *p);
    }
}

/**
 * @brief helper to append a new line containing a string to a buffer.
 * @param buf pointer to the buffer.
 * @param row the row index the new line will occupy.
 * @param s null-terminated string to place on the new line.
 */
static void append_line(Buffer *buf, int row, const char *s)
{
    /* split off a new empty line after the previous one */
    buffer_insert_newline(buf, row - 1, buf->lines[row - 1].len);
    int col = 0;
    for (const char *p = s; *p != '\0'; p++)
    {
        buffer_insert_char(buf, row, col++, *p);
    }
}

TEST(search_find_in_line_basic)
{
    Buffer *buf = buffer_create();
    fill_line(buf, "hello world hello");

    int col = -1;
    ASSERT(search_find_in_line(&buf->lines[0], "hello", 0, &col));
    ASSERT_EQ(col, 0);

    /* start past the first match to find the second */
    ASSERT(search_find_in_line(&buf->lines[0], "hello", 1, &col));
    ASSERT_EQ(col, 12);

    buffer_free(buf);
}

TEST(search_find_in_line_no_match)
{
    Buffer *buf = buffer_create();
    fill_line(buf, "abcdef");
    int col = -1;
    ASSERT_EQ(search_find_in_line(&buf->lines[0], "xyz", 0, &col), 0);
    buffer_free(buf);
}

TEST(search_empty_query_never_matches)
{
    Buffer *buf = buffer_create();
    fill_line(buf, "abc");

    SearchMatch m;
    ASSERT_EQ(search_find_next(buf, "", 0, 0, &m), 0);
    ASSERT_EQ(search_find_prev(buf, "", 0, 0, &m), 0);
    ASSERT_EQ(search_find_in_line(&buf->lines[0], "", 0, NULL), 0);
    ASSERT_EQ(search_count_in_line(&buf->lines[0], ""), 0);
    buffer_free(buf);
}

TEST(search_count_in_line_counts_non_overlapping)
{
    Buffer *buf = buffer_create();
    fill_line(buf, "aaaa");
    /* "aa" occurs non-overlapping twice in "aaaa" */
    ASSERT_EQ(search_count_in_line(&buf->lines[0], "aa"), 2);

    buffer_free(buf);
}

TEST(search_find_next_forward_same_line)
{
    Buffer *buf = buffer_create();
    fill_line(buf, "foo bar foo");

    SearchMatch m;
    ASSERT(search_find_next(buf, "foo", 0, 0, &m));
    ASSERT_EQ(m.row, 0);
    ASSERT_EQ(m.col, 0);
    ASSERT_EQ(m.len, 3);

    /* searching from col 1 should skip to the second occurrence */
    ASSERT(search_find_next(buf, "foo", 0, 1, &m));
    ASSERT_EQ(m.col, 8);
    buffer_free(buf);
}

TEST(search_find_next_across_lines)
{
    Buffer *buf = buffer_create();
    fill_line(buf, "first line");
    append_line(buf, 1, "second target");
    append_line(buf, 2, "third line");

    SearchMatch m;
    ASSERT(search_find_next(buf, "target", 0, 0, &m));
    ASSERT_EQ(m.row, 1);
    ASSERT_EQ(m.col, 7);
    buffer_free(buf);
}

TEST(search_find_next_wraps_around)
{
    Buffer *buf = buffer_create();
    fill_line(buf, "match here");
    append_line(buf, 1, "nothing");

    /* start on the last line: the only match is above, so it must wrap */
    SearchMatch m;
    ASSERT(search_find_next(buf, "match", 1, 0, &m));
    ASSERT_EQ(m.row, 0);
    ASSERT_EQ(m.col, 0);
    buffer_free(buf);
}

TEST(search_find_next_no_match_returns_zero)
{
    Buffer *buf = buffer_create();
    fill_line(buf, "one");
    append_line(buf, 1, "two");

    SearchMatch m;
    ASSERT_EQ(search_find_next(buf, "zzz", 0, 0, &m), 0);
    buffer_free(buf);
}

TEST(search_find_prev_backward_same_line)
{
    Buffer *buf = buffer_create();
    fill_line(buf, "foo bar foo baz");

    /* from the end, previous match before col 15 is the second "foo" at 8 */
    SearchMatch m;
    ASSERT(search_find_prev(buf, "foo", 0, 15, &m));
    ASSERT_EQ(m.row, 0);
    ASSERT_EQ(m.col, 8);

    /* now search before that match: should find the first "foo" at 0 */
    ASSERT(search_find_prev(buf, "foo", 0, 8, &m));
    ASSERT_EQ(m.col, 0);
    buffer_free(buf);
}

TEST(search_find_prev_across_lines)
{
    Buffer *buf = buffer_create();
    fill_line(buf, "alpha target");
    append_line(buf, 1, "beta");
    append_line(buf, 2, "gamma");

    /* from row 2 col 0, previous match is on row 0 */
    SearchMatch m;
    ASSERT(search_find_prev(buf, "target", 2, 0, &m));
    ASSERT_EQ(m.row, 0);
    ASSERT_EQ(m.col, 6);
    buffer_free(buf);
}

TEST(search_find_prev_wraps_around)
{
    Buffer *buf = buffer_create();
    fill_line(buf, "nothing");
    append_line(buf, 1, "the needle");

    /* from row 0 col 0, the only match is below, so it must wrap to row 1 */
    SearchMatch m;
    ASSERT(search_find_prev(buf, "needle", 0, 0, &m));
    ASSERT_EQ(m.row, 1);
    ASSERT_EQ(m.col, 4);
    buffer_free(buf);
}

TEST(search_next_prev_are_symmetric)
{
    Buffer *buf = buffer_create();
    fill_line(buf, "x");
    append_line(buf, 1, "hit");
    append_line(buf, 2, "y");
    append_line(buf, 3, "hit");

    /* forward from top finds the first hit on row 1 */
    SearchMatch fwd;
    ASSERT(search_find_next(buf, "hit", 0, 0, &fwd));
    ASSERT_EQ(fwd.row, 1);

    /* forward again from just past it finds row 3 */
    ASSERT(search_find_next(buf, "hit", fwd.row, fwd.col + 1, &fwd));
    ASSERT_EQ(fwd.row, 3);

    /* backward from row 3 col 0 returns to row 1 */
    SearchMatch bwd;
    ASSERT(search_find_prev(buf, "hit", 3, 0, &bwd));
    ASSERT_EQ(bwd.row, 1);
    buffer_free(buf);
}
