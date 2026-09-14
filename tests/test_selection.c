/**
 * @file test_selection.c
 * @brief unit tests for the selection module and its command integration.
 *
 * two layers are covered: the selection module in isolation (normalize,
 * contains, copy, and region deletion against a buffer with undo/redo) and the
 * integration through editor_execute, which exercises the select commands and
 * confirms that a plain movement clears an active selection.
 */

#include "selection.h"
#include "command.h"
#include "editor.h"
#include "undo.h"

#include <stdlib.h>
#include <string.h>

/**
 * @brief build a minimal Editor around a fresh buffer without touching the
 *        terminal, so command execution can be tested in isolation.
 * @param ed pointer to the Editor to initialize.
 */
static void sel_editor_init(Editor *ed)
{
    ed->buffer = buffer_create();
    ed->cursor.row = 0;
    ed->cursor.col = 0;
    ed->scroll_offset = 0;
    ed->screen_rows = 24;
    ed->screen_cols = 80;
    ed->running = 1;
    ed->filename = NULL;
    history_init(&ed->history);
    selection_clear(&ed->selection);
}

/**
 * @brief release everything owned by a test editor.
 * @param ed pointer to the Editor to tear down.
 */
static void sel_editor_free(Editor *ed)
{
    buffer_free(ed->buffer);
    history_free(&ed->history);
}

/**
 * @brief type a string into a buffer, splitting lines on newline characters.
 * @param buf pointer to the Buffer to fill.
 * @param s the null-terminated string to insert.
 */
static void fill_buffer(Buffer *buf, const char *s)
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

/**
 * @brief compare a buffer line against an expected string.
 * @param buf pointer to the Buffer.
 * @param row the row index to read.
 * @param expected the expected content of the line.
 * @return 1 if the line matches, 0 otherwise.
 */
static int line_equals(const Buffer *buf, int row, const char *expected)
{
    int len = (int)strlen(expected);
    if (buf->lines[row].len != len)
        return 0;
    return memcmp(buf->lines[row].chars, expected, len) == 0;
}

TEST(selection_starts_inactive_after_clear)
{
    Selection sel;
    selection_clear(&sel);
    ASSERT_EQ(sel.active, 0);
    ASSERT_EQ(selection_contains(&sel, 0, 0), 0);
}

TEST(selection_start_marks_active_at_point)
{
    Selection sel;
    selection_clear(&sel);
    selection_start(&sel, 2, 5);
    ASSERT_EQ(sel.active, 1);
    ASSERT_EQ(sel.anchor_row, 2);
    ASSERT_EQ(sel.anchor_col, 5);
    ASSERT_EQ(sel.cursor_row, 2);
    ASSERT_EQ(sel.cursor_col, 5);
}

TEST(selection_set_cursor_keeps_anchor_fixed)
{
    Selection sel;
    selection_clear(&sel);
    selection_start(&sel, 1, 3);
    selection_set_cursor(&sel, 1, 7);
    ASSERT_EQ(sel.anchor_row, 1);
    ASSERT_EQ(sel.anchor_col, 3);
    ASSERT_EQ(sel.cursor_row, 1);
    ASSERT_EQ(sel.cursor_col, 7);
}

TEST(selection_set_cursor_ignored_when_inactive)
{
    Selection sel;
    selection_clear(&sel);
    selection_set_cursor(&sel, 4, 4);
    ASSERT_EQ(sel.active, 0);
    ASSERT_EQ(sel.cursor_row, 0);
    ASSERT_EQ(sel.cursor_col, 0);
}

TEST(selection_normalize_forward)
{
    /* anchor before cursor: order is unchanged */
    Selection sel;
    selection_clear(&sel);
    selection_start(&sel, 1, 2);
    selection_set_cursor(&sel, 3, 4);
    int sr, sc, er, ec;
    selection_normalize(&sel, &sr, &sc, &er, &ec);
    ASSERT_EQ(sr, 1);
    ASSERT_EQ(sc, 2);
    ASSERT_EQ(er, 3);
    ASSERT_EQ(ec, 4);
}

TEST(selection_normalize_backward)
{
    /* cursor before anchor: the two points swap */
    Selection sel;
    selection_clear(&sel);
    selection_start(&sel, 3, 4);
    selection_set_cursor(&sel, 1, 2);
    int sr, sc, er, ec;
    selection_normalize(&sel, &sr, &sc, &er, &ec);
    ASSERT_EQ(sr, 1);
    ASSERT_EQ(sc, 2);
    ASSERT_EQ(er, 3);
    ASSERT_EQ(ec, 4);
}

TEST(selection_normalize_same_row_swaps_columns)
{
    Selection sel;
    selection_clear(&sel);
    selection_start(&sel, 2, 8);
    selection_set_cursor(&sel, 2, 3);
    int sr, sc, er, ec;
    selection_normalize(&sel, &sr, &sc, &er, &ec);
    ASSERT_EQ(sr, 2);
    ASSERT_EQ(sc, 3);
    ASSERT_EQ(er, 2);
    ASSERT_EQ(ec, 8);
}

TEST(selection_contains_is_half_open)
{
    /* region from (0,1) to (0,4) covers columns 1,2,3 but not 4 */
    Selection sel;
    selection_clear(&sel);
    selection_start(&sel, 0, 1);
    selection_set_cursor(&sel, 0, 4);
    ASSERT_EQ(selection_contains(&sel, 0, 0), 0);
    ASSERT_EQ(selection_contains(&sel, 0, 1), 1);
    ASSERT_EQ(selection_contains(&sel, 0, 3), 1);
    ASSERT_EQ(selection_contains(&sel, 0, 4), 0);
}

TEST(selection_contains_spans_multiple_rows)
{
    /* region from (0,2) to (2,1) */
    Selection sel;
    selection_clear(&sel);
    selection_start(&sel, 0, 2);
    selection_set_cursor(&sel, 2, 1);
    ASSERT_EQ(selection_contains(&sel, 0, 1), 0);
    ASSERT_EQ(selection_contains(&sel, 0, 2), 1);
    ASSERT_EQ(selection_contains(&sel, 1, 0), 1); /* full middle row */
    ASSERT_EQ(selection_contains(&sel, 1, 99), 1);
    ASSERT_EQ(selection_contains(&sel, 2, 0), 1);
    ASSERT_EQ(selection_contains(&sel, 2, 1), 0); /* end point excluded */
}

TEST(selection_copy_region_single_line)
{
    Buffer *buf = buffer_create();
    fill_buffer(buf, "hello world");
    Selection sel;
    selection_clear(&sel);
    selection_start(&sel, 0, 6);
    selection_set_cursor(&sel, 0, 11);
    int len = 0;
    char *text = selection_copy_region(&sel, buf, &len);
    ASSERT(text != NULL);
    ASSERT_EQ(len, 5);
    ASSERT_STR_EQ(text, "world");
    free(text);
    buffer_free(buf);
}

TEST(selection_copy_region_multi_line)
{
    Buffer *buf = buffer_create();
    fill_buffer(buf, "abc\ndef\nghi");
    Selection sel;
    selection_clear(&sel);
    /* from (0,1) to (2,2): "bc\ndef\ngh" */
    selection_start(&sel, 0, 1);
    selection_set_cursor(&sel, 2, 2);
    int len = 0;
    char *text = selection_copy_region(&sel, buf, &len);
    ASSERT(text != NULL);
    ASSERT_STR_EQ(text, "bc\ndef\ngh");
    ASSERT_EQ(len, 9);
    free(text);
    buffer_free(buf);
}

TEST(selection_copy_region_inactive_returns_null)
{
    Buffer *buf = buffer_create();
    fill_buffer(buf, "abc");
    Selection sel;
    selection_clear(&sel);
    int len = 5;
    char *text = selection_copy_region(&sel, buf, &len);
    ASSERT(text == NULL);
    ASSERT_EQ(len, 0);
    buffer_free(buf);
}

TEST(selection_delete_region_single_line)
{
    Buffer *buf = buffer_create();
    fill_buffer(buf, "hello world");
    History h;
    history_init(&h);
    Selection sel;
    selection_clear(&sel);
    selection_start(&sel, 0, 5);
    selection_set_cursor(&sel, 0, 11); /* delete " world" */
    int r = -1, c = -1;
    int ok = selection_delete_region(&sel, buf, &h, &r, &c);
    ASSERT_EQ(ok, 1);
    ASSERT_EQ(buf->num_lines, 1);
    ASSERT(line_equals(buf, 0, "hello"));
    ASSERT_EQ(r, 0);
    ASSERT_EQ(c, 5);
    history_free(&h);
    buffer_free(buf);
}

TEST(selection_delete_region_multi_line)
{
    Buffer *buf = buffer_create();
    fill_buffer(buf, "abc\ndef\nghi");
    History h;
    history_init(&h);
    Selection sel;
    selection_clear(&sel);
    /* delete from (0,1) to (2,2): leaves "a" + "i" joined -> "ai" */
    selection_start(&sel, 0, 1);
    selection_set_cursor(&sel, 2, 2);
    int r = -1, c = -1;
    int ok = selection_delete_region(&sel, buf, &h, &r, &c);
    ASSERT_EQ(ok, 1);
    ASSERT_EQ(buf->num_lines, 1);
    ASSERT(line_equals(buf, 0, "ai"));
    ASSERT_EQ(r, 0);
    ASSERT_EQ(c, 1);
    history_free(&h);
    buffer_free(buf);
}

TEST(selection_delete_region_undo_restores_single_line)
{
    Buffer *buf = buffer_create();
    fill_buffer(buf, "hello world");
    History h;
    history_init(&h);
    Selection sel;
    selection_clear(&sel);
    selection_start(&sel, 0, 5);
    selection_set_cursor(&sel, 0, 11);
    int r, c;
    selection_delete_region(&sel, buf, &h, &r, &c);
    ASSERT(line_equals(buf, 0, "hello"));

    /* one undo restores the whole region */
    int ur, uc;
    ASSERT_EQ(history_undo(&h, buf, &ur, &uc), 1);
    ASSERT_EQ(buf->num_lines, 1);
    ASSERT(line_equals(buf, 0, "hello world"));

    /* one redo removes it again */
    int rr, rc;
    ASSERT_EQ(history_redo(&h, buf, &rr, &rc), 1);
    ASSERT(line_equals(buf, 0, "hello"));
    history_free(&h);
    buffer_free(buf);
}

TEST(selection_delete_region_undo_restores_multi_line)
{
    Buffer *buf = buffer_create();
    fill_buffer(buf, "abc\ndef\nghi");
    History h;
    history_init(&h);
    Selection sel;
    selection_clear(&sel);
    selection_start(&sel, 0, 1);
    selection_set_cursor(&sel, 2, 2);
    int r, c;
    selection_delete_region(&sel, buf, &h, &r, &c);
    ASSERT_EQ(buf->num_lines, 1);
    ASSERT(line_equals(buf, 0, "ai"));

    /* one undo restores all three original lines */
    int ur, uc;
    ASSERT_EQ(history_undo(&h, buf, &ur, &uc), 1);
    ASSERT_EQ(buf->num_lines, 3);
    ASSERT(line_equals(buf, 0, "abc"));
    ASSERT(line_equals(buf, 1, "def"));
    ASSERT(line_equals(buf, 2, "ghi"));

    /* one redo collapses them again */
    int rr, rc;
    ASSERT_EQ(history_redo(&h, buf, &rr, &rc), 1);
    ASSERT_EQ(buf->num_lines, 1);
    ASSERT(line_equals(buf, 0, "ai"));
    history_free(&h);
    buffer_free(buf);
}

TEST(selection_delete_region_inactive_is_noop)
{
    Buffer *buf = buffer_create();
    fill_buffer(buf, "abc");
    History h;
    history_init(&h);
    Selection sel;
    selection_clear(&sel);
    int r = -1, c = -1;
    int ok = selection_delete_region(&sel, buf, &h, &r, &c);
    ASSERT_EQ(ok, 0);
    ASSERT(line_equals(buf, 0, "abc"));
    history_free(&h);
    buffer_free(buf);
}

TEST(cmd_select_right_starts_and_extends_selection)
{
    Editor ed;
    sel_editor_init(&ed);
    fill_buffer(ed.buffer, "hello");
    ed.cursor.row = 0;
    ed.cursor.col = 0;

    Command sel_right = {CMD_SELECT_RIGHT, 0};
    editor_execute(&ed, sel_right);
    editor_execute(&ed, sel_right);

    ASSERT_EQ(ed.selection.active, 1);
    ASSERT_EQ(ed.selection.anchor_row, 0);
    ASSERT_EQ(ed.selection.anchor_col, 0);
    ASSERT_EQ(ed.cursor.col, 2);
    ASSERT_EQ(ed.selection.cursor_col, 2);
    ASSERT_EQ(selection_contains(&ed.selection, 0, 0), 1);
    ASSERT_EQ(selection_contains(&ed.selection, 0, 1), 1);
    ASSERT_EQ(selection_contains(&ed.selection, 0, 2), 0);

    sel_editor_free(&ed);
}

TEST(cmd_move_clears_active_selection)
{
    Editor ed;
    sel_editor_init(&ed);
    fill_buffer(ed.buffer, "hello");
    ed.cursor.row = 0;
    ed.cursor.col = 0;

    Command sel_right = {CMD_SELECT_RIGHT, 0};
    editor_execute(&ed, sel_right);
    ASSERT_EQ(ed.selection.active, 1);

    Command move_left = {CMD_MOVE_LEFT, 0};
    editor_execute(&ed, move_left);
    ASSERT_EQ(ed.selection.active, 0);

    sel_editor_free(&ed);
}

TEST(cmd_select_all_covers_whole_buffer)
{
    Editor ed;
    sel_editor_init(&ed);
    fill_buffer(ed.buffer, "abc\ndef");

    Command select_all = {CMD_SELECT_ALL, 0};
    editor_execute(&ed, select_all);

    ASSERT_EQ(ed.selection.active, 1);
    ASSERT_EQ(ed.selection.anchor_row, 0);
    ASSERT_EQ(ed.selection.anchor_col, 0);
    ASSERT_EQ(ed.selection.cursor_row, 1);
    ASSERT_EQ(ed.selection.cursor_col, 3);
    ASSERT_EQ(ed.cursor.row, 1);
    ASSERT_EQ(ed.cursor.col, 3);

    sel_editor_free(&ed);
}

TEST(cmd_insert_char_clears_selection)
{
    Editor ed;
    sel_editor_init(&ed);
    fill_buffer(ed.buffer, "hello");
    ed.cursor.row = 0;
    ed.cursor.col = 0;

    Command sel_right = {CMD_SELECT_RIGHT, 0};
    editor_execute(&ed, sel_right);
    ASSERT_EQ(ed.selection.active, 1);

    Command insert = {CMD_INSERT_CHAR, 'x'};
    editor_execute(&ed, insert);
    ASSERT_EQ(ed.selection.active, 0);

    sel_editor_free(&ed);
}

TEST(cmd_move_word_right_advances_and_clears_selection)
{
    Editor ed;
    sel_editor_init(&ed);
    fill_buffer(ed.buffer, "hello world");
    ed.cursor.row = 0;
    ed.cursor.col = 0;

    /* start a selection, then a plain word move must clear it */
    Command sel_right = {CMD_SELECT_RIGHT, 0};
    editor_execute(&ed, sel_right);
    ASSERT_EQ(ed.selection.active, 1);

    Command word_right = {CMD_MOVE_WORD_RIGHT, 0};
    editor_execute(&ed, word_right);
    ASSERT_EQ(ed.selection.active, 0);
    ASSERT_EQ(ed.cursor.col, 5);

    sel_editor_free(&ed);
}

TEST(cmd_move_word_left_retreats_and_clears_selection)
{
    Editor ed;
    sel_editor_init(&ed);
    fill_buffer(ed.buffer, "hello world");
    ed.cursor.row = 0;
    ed.cursor.col = 11;

    Command word_left = {CMD_MOVE_WORD_LEFT, 0};
    editor_execute(&ed, word_left);
    ASSERT_EQ(ed.selection.active, 0);
    ASSERT_EQ(ed.cursor.col, 6);

    sel_editor_free(&ed);
}

TEST(cmd_select_word_right_extends_selection)
{
    Editor ed;
    sel_editor_init(&ed);
    fill_buffer(ed.buffer, "hello world");
    ed.cursor.row = 0;
    ed.cursor.col = 0;

    Command sel_word_right = {CMD_SELECT_WORD_RIGHT, 0};
    editor_execute(&ed, sel_word_right);

    ASSERT_EQ(ed.selection.active, 1);
    ASSERT_EQ(ed.selection.anchor_col, 0);
    ASSERT_EQ(ed.cursor.col, 5);
    ASSERT_EQ(ed.selection.cursor_col, 5);
    /* the whole word "hello" is now selected */
    ASSERT_EQ(selection_contains(&ed.selection, 0, 0), 1);
    ASSERT_EQ(selection_contains(&ed.selection, 0, 4), 1);
    ASSERT_EQ(selection_contains(&ed.selection, 0, 5), 0);

    sel_editor_free(&ed);
}

TEST(cmd_select_word_left_extends_selection)
{
    Editor ed;
    sel_editor_init(&ed);
    fill_buffer(ed.buffer, "hello world");
    ed.cursor.row = 0;
    ed.cursor.col = 11;

    Command sel_word_left = {CMD_SELECT_WORD_LEFT, 0};
    editor_execute(&ed, sel_word_left);

    ASSERT_EQ(ed.selection.active, 1);
    ASSERT_EQ(ed.selection.anchor_col, 11);
    ASSERT_EQ(ed.cursor.col, 6);
    /* the word "world" is now selected (columns 6..10) */
    ASSERT_EQ(selection_contains(&ed.selection, 0, 6), 1);
    ASSERT_EQ(selection_contains(&ed.selection, 0, 10), 1);

    sel_editor_free(&ed);
}
