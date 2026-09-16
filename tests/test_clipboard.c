/**
 * @file test_clipboard.c
 * @brief unit tests for the clipboard module and its command integration.
 *
 * two layers are covered: the clipboard module in isolation (store, empty
 * test, and paste against a buffer with undo/redo) and the integration through
 * editor_execute, which exercises copy, cut, and paste with the selection.
 */

#include "clipboard.h"
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
static void clip_editor_init(Editor *ed)
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
    clipboard_init(&ed->clipboard);
}

/**
 * @brief release everything owned by a test editor.
 * @param ed pointer to the Editor to tear down.
 */
static void clip_editor_free(Editor *ed)
{
    buffer_free(ed->buffer);
    history_free(&ed->history);
    clipboard_free(&ed->clipboard);
}

/**
 * @brief type a string into a buffer, splitting lines on newline characters.
 * @param buf pointer to the Buffer to fill.
 * @param s the null-terminated string to insert.
 */
static void clip_fill_buffer(Buffer *buf, const char *s)
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
static int clip_line_equals(const Buffer *buf, int row, const char *expected)
{
    int len = (int)strlen(expected);
    if (buf->lines[row].len != len)
        return 0;
    return memcmp(buf->lines[row].chars, expected, len) == 0;
}

TEST(clipboard_starts_empty)
{
    Clipboard cb;
    clipboard_init(&cb);
    ASSERT_EQ(clipboard_is_empty(&cb), 1);
    clipboard_free(&cb);
}

TEST(clipboard_set_stores_a_copy)
{
    Clipboard cb;
    clipboard_init(&cb);
    char src[] = "hello";
    clipboard_set(&cb, src, 5);
    /* mutate the source to prove the clipboard kept a private copy */
    src[0] = 'x';
    ASSERT_EQ(clipboard_is_empty(&cb), 0);
    ASSERT_EQ(cb.len, 5);
    ASSERT_STR_EQ(cb.text, "hello");
    clipboard_free(&cb);
}

TEST(clipboard_set_empty_clears)
{
    Clipboard cb;
    clipboard_init(&cb);
    clipboard_set(&cb, "abc", 3);
    clipboard_set(&cb, NULL, 0);
    ASSERT_EQ(clipboard_is_empty(&cb), 1);
    clipboard_free(&cb);
}

TEST(clipboard_paste_empty_is_noop)
{
    Buffer *buf = buffer_create();
    clip_fill_buffer(buf, "abc");
    History h;
    history_init(&h);
    Clipboard cb;
    clipboard_init(&cb);
    int r = -1, c = -1;
    int ok = clipboard_paste(&cb, buf, &h, 0, 3, &r, &c);
    ASSERT_EQ(ok, 0);
    ASSERT(clip_line_equals(buf, 0, "abc"));
    clipboard_free(&cb);
    history_free(&h);
    buffer_free(buf);
}

TEST(clipboard_paste_single_line)
{
    Buffer *buf = buffer_create();
    clip_fill_buffer(buf, "helloworld");
    History h;
    history_init(&h);
    Clipboard cb;
    clipboard_init(&cb);
    clipboard_set(&cb, " brave ", 7);
    int r, c;
    int ok = clipboard_paste(&cb, buf, &h, 0, 5, &r, &c);
    ASSERT_EQ(ok, 1);
    ASSERT(clip_line_equals(buf, 0, "hello brave world"));
    ASSERT_EQ(r, 0);
    ASSERT_EQ(c, 12);
    clipboard_free(&cb);
    history_free(&h);
    buffer_free(buf);
}

TEST(clipboard_paste_multi_line)
{
    Buffer *buf = buffer_create();
    clip_fill_buffer(buf, "ac");
    History h;
    history_init(&h);
    Clipboard cb;
    clipboard_init(&cb);
    /* paste "X\nY" between 'a' and 'c' -> "aX", "Yc" */
    clipboard_set(&cb, "X\nY", 3);
    int r, c;
    int ok = clipboard_paste(&cb, buf, &h, 0, 1, &r, &c);
    ASSERT_EQ(ok, 1);
    ASSERT_EQ(buf->num_lines, 2);
    ASSERT(clip_line_equals(buf, 0, "aX"));
    ASSERT(clip_line_equals(buf, 1, "Yc"));
    ASSERT_EQ(r, 1);
    ASSERT_EQ(c, 1);
    clipboard_free(&cb);
    history_free(&h);
    buffer_free(buf);
}

TEST(clipboard_paste_single_line_undo_redo)
{
    Buffer *buf = buffer_create();
    clip_fill_buffer(buf, "helloworld");
    History h;
    history_init(&h);
    Clipboard cb;
    clipboard_init(&cb);
    clipboard_set(&cb, " brave ", 7);
    int r, c;
    clipboard_paste(&cb, buf, &h, 0, 5, &r, &c);
    ASSERT(clip_line_equals(buf, 0, "hello brave world"));

    /* one undo removes the whole pasted text */
    int ur, uc;
    ASSERT_EQ(history_undo(&h, buf, &ur, &uc), 1);
    ASSERT(clip_line_equals(buf, 0, "helloworld"));

    /* one redo reinserts it */
    int rr, rc;
    ASSERT_EQ(history_redo(&h, buf, &rr, &rc), 1);
    ASSERT(clip_line_equals(buf, 0, "hello brave world"));
    clipboard_free(&cb);
    history_free(&h);
    buffer_free(buf);
}

TEST(clipboard_paste_multi_line_undo_redo)
{
    Buffer *buf = buffer_create();
    clip_fill_buffer(buf, "ac");
    History h;
    history_init(&h);
    Clipboard cb;
    clipboard_init(&cb);
    clipboard_set(&cb, "X\nY", 3);
    int r, c;
    clipboard_paste(&cb, buf, &h, 0, 1, &r, &c);
    ASSERT_EQ(buf->num_lines, 2);

    /* one undo collapses the paste back to a single line */
    int ur, uc;
    ASSERT_EQ(history_undo(&h, buf, &ur, &uc), 1);
    ASSERT_EQ(buf->num_lines, 1);
    ASSERT(clip_line_equals(buf, 0, "ac"));

    /* one redo restores the two lines */
    int rr, rc;
    ASSERT_EQ(history_redo(&h, buf, &rr, &rc), 1);
    ASSERT_EQ(buf->num_lines, 2);
    ASSERT(clip_line_equals(buf, 0, "aX"));
    ASSERT(clip_line_equals(buf, 1, "Yc"));
    clipboard_free(&cb);
    history_free(&h);
    buffer_free(buf);
}

TEST(cmd_copy_fills_clipboard_without_changing_buffer)
{
    Editor ed;
    clip_editor_init(&ed);
    clip_fill_buffer(ed.buffer, "hello world");
    selection_start(&ed.selection, 0, 6);
    selection_set_cursor(&ed.selection, 0, 11);

    Command copy = {CMD_COPY, 0};
    editor_execute(&ed, copy);

    ASSERT_EQ(clipboard_is_empty(&ed.clipboard), 0);
    ASSERT_STR_EQ(ed.clipboard.text, "world");
    /* buffer unchanged and selection still active */
    ASSERT(clip_line_equals(ed.buffer, 0, "hello world"));
    ASSERT_EQ(ed.selection.active, 1);

    clip_editor_free(&ed);
}

TEST(cmd_cut_fills_clipboard_and_deletes_region)
{
    Editor ed;
    clip_editor_init(&ed);
    clip_fill_buffer(ed.buffer, "hello world");
    selection_start(&ed.selection, 0, 5);
    selection_set_cursor(&ed.selection, 0, 11);

    Command cut = {CMD_CUT, 0};
    editor_execute(&ed, cut);

    ASSERT_STR_EQ(ed.clipboard.text, " world");
    ASSERT(clip_line_equals(ed.buffer, 0, "hello"));
    ASSERT_EQ(ed.selection.active, 0);
    ASSERT_EQ(ed.cursor.col, 5);

    /* one undo restores the cut text */
    Command undo = {CMD_UNDO, 0};
    editor_execute(&ed, undo);
    ASSERT(clip_line_equals(ed.buffer, 0, "hello world"));

    clip_editor_free(&ed);
}

TEST(cmd_paste_inserts_clipboard_at_cursor)
{
    Editor ed;
    clip_editor_init(&ed);
    clip_fill_buffer(ed.buffer, "helloworld");
    clipboard_set(&ed.clipboard, " ", 1);
    ed.cursor.row = 0;
    ed.cursor.col = 5;

    Command paste = {CMD_PASTE, 0};
    editor_execute(&ed, paste);

    ASSERT(clip_line_equals(ed.buffer, 0, "hello world"));
    ASSERT_EQ(ed.cursor.col, 6);

    clip_editor_free(&ed);
}

TEST(cmd_cut_then_paste_round_trips)
{
    Editor ed;
    clip_editor_init(&ed);
    clip_fill_buffer(ed.buffer, "hello world");
    selection_start(&ed.selection, 0, 5);
    selection_set_cursor(&ed.selection, 0, 11);

    Command cut = {CMD_CUT, 0};
    editor_execute(&ed, cut);
    ASSERT(clip_line_equals(ed.buffer, 0, "hello"));

    /* cursor is at column 5; paste puts the cut text back */
    Command paste = {CMD_PASTE, 0};
    editor_execute(&ed, paste);
    ASSERT(clip_line_equals(ed.buffer, 0, "hello world"));

    clip_editor_free(&ed);
}

TEST(cmd_paste_over_selection_replaces_it)
{
    Editor ed;
    clip_editor_init(&ed);
    clip_fill_buffer(ed.buffer, "hello world");
    clipboard_set(&ed.clipboard, "there", 5);
    /* select "world" and paste over it */
    selection_start(&ed.selection, 0, 6);
    selection_set_cursor(&ed.selection, 0, 11);

    Command paste = {CMD_PASTE, 0};
    editor_execute(&ed, paste);

    ASSERT(clip_line_equals(ed.buffer, 0, "hello there"));
    ASSERT_EQ(ed.selection.active, 0);

    clip_editor_free(&ed);
}
