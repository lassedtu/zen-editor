/**
 * @file test_undo.c
 * @brief unit tests for the undo/redo module and its command integration.
 *
 * two layers are covered: the undo module in isolation (recording entries and
 * applying them to a buffer) and the integration through editor_execute, which
 * exercises the real recording paths for each editing command.
 */

#include "undo.h"
#include "command.h"
#include "editor.h"

#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* helpers                                                            */
/* ------------------------------------------------------------------ */

/**
 * @brief build a minimal Editor around a fresh buffer without touching the
 *        terminal, so command execution can be tested in isolation.
 * @param ed pointer to the Editor to initialize.
 */
static void test_editor_init(Editor *ed)
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
}

/**
 * @brief release everything owned by a test editor.
 * @param ed pointer to the Editor to tear down.
 */
static void test_editor_free(Editor *ed)
{
    buffer_free(ed->buffer);
    history_free(&ed->history);
}

/**
 * @brief type a string into the editor one character at a time.
 * @param ed pointer to the Editor.
 * @param s the null-terminated string to type.
 */
static void type_str(Editor *ed, const char *s)
{
    for (const char *p = s; *p; p++)
    {
        Command cmd = {CMD_INSERT_CHAR, (unsigned char)*p};
        editor_execute(ed, cmd);
    }
}

/* ------------------------------------------------------------------ */
/* module-level tests                                                 */
/* ------------------------------------------------------------------ */

TEST(history_init_starts_empty)
{
    History h;
    history_init(&h);
    ASSERT_EQ(h.undo.count, 0);
    ASSERT_EQ(h.redo.count, 0);
    history_free(&h);
}

TEST(history_undo_on_empty_returns_zero)
{
    Buffer *buf = buffer_create();
    History h;
    history_init(&h);

    int r = 5, c = 7;
    int result = history_undo(&h, buf, &r, &c);
    ASSERT_EQ(result, 0);
    /* out params untouched when there is nothing to undo */
    ASSERT_EQ(r, 5);
    ASSERT_EQ(c, 7);

    history_free(&h);
    buffer_free(buf);
}

TEST(history_redo_on_empty_returns_zero)
{
    Buffer *buf = buffer_create();
    History h;
    history_init(&h);

    int r = 0, c = 0;
    ASSERT_EQ(history_redo(&h, buf, &r, &c), 0);

    history_free(&h);
    buffer_free(buf);
}

TEST(history_record_clears_redo_stack)
{
    /* recording a fresh edit must invalidate any pending redo history */
    Buffer *buf = buffer_create();
    buffer_insert_char(buf, 0, 0, 'a');
    History h;
    history_init(&h);

    UndoEntry e;
    e.num_ops = 1;
    e.ops[0] = (UndoOp){UNDO_OP_DELETE_CHAR, 0, 0, 0};
    e.cursor_row = 0;
    e.cursor_col = 0;
    e.redo_row = 0;
    e.redo_col = 1;
    history_record(&h, e);

    int r, c;
    history_undo(&h, buf, &r, &c);      /* moves entry to redo stack */
    ASSERT_EQ(h.redo.count, 1);

    history_record(&h, e);              /* fresh edit clears redo */
    ASSERT_EQ(h.redo.count, 0);

    history_free(&h);
    buffer_free(buf);
}

TEST(history_cap_drops_oldest_entry)
{
    /* pushing more than UNDO_MAX_ENTRIES keeps the stack capped */
    Buffer *buf = buffer_create();
    History h;
    history_init(&h);

    UndoEntry e;
    e.num_ops = 1;
    e.ops[0] = (UndoOp){UNDO_OP_DELETE_CHAR, 0, 0, 0};
    e.cursor_row = 0;
    e.cursor_col = 0;
    e.redo_row = 0;
    e.redo_col = 0;

    for (int i = 0; i < UNDO_MAX_ENTRIES + 50; i++)
    {
        history_record(&h, e);
    }
    ASSERT_EQ(h.undo.count, UNDO_MAX_ENTRIES);

    history_free(&h);
    buffer_free(buf);
}

/* ------------------------------------------------------------------ */
/* integration tests via editor_execute                               */
/* ------------------------------------------------------------------ */

TEST(undo_insert_char_removes_it)
{
    Editor ed;
    test_editor_init(&ed);

    type_str(&ed, "hi");
    ASSERT_EQ(ed.buffer->lines[0].len, 2);

    Command undo = {CMD_UNDO, 0};
    editor_execute(&ed, undo);
    ASSERT_EQ(ed.buffer->lines[0].len, 1);
    ASSERT(ed.buffer->lines[0].chars[0] == 'h');
    /* cursor returns to where it was before the undone insert */
    ASSERT_EQ(ed.cursor.col, 1);

    editor_execute(&ed, undo);
    ASSERT_EQ(ed.buffer->lines[0].len, 0);
    ASSERT_EQ(ed.cursor.col, 0);

    test_editor_free(&ed);
}

TEST(redo_reapplies_insert)
{
    Editor ed;
    test_editor_init(&ed);

    type_str(&ed, "ab");
    Command undo = {CMD_UNDO, 0};
    Command redo = {CMD_REDO, 0};

    editor_execute(&ed, undo);   /* remove 'b' */
    editor_execute(&ed, undo);   /* remove 'a' */
    ASSERT_EQ(ed.buffer->lines[0].len, 0);

    editor_execute(&ed, redo);   /* restore 'a' */
    ASSERT_EQ(ed.buffer->lines[0].len, 1);
    ASSERT(ed.buffer->lines[0].chars[0] == 'a');
    ASSERT_EQ(ed.cursor.col, 1);

    editor_execute(&ed, redo);   /* restore 'b' */
    ASSERT_EQ(ed.buffer->lines[0].len, 2);
    ASSERT(ed.buffer->lines[0].chars[1] == 'b');
    ASSERT_EQ(ed.cursor.col, 2);

    test_editor_free(&ed);
}

TEST(undo_delete_char_restores_it)
{
    Editor ed;
    test_editor_init(&ed);

    type_str(&ed, "abc");
    ed.cursor.col = 1;                 /* on 'b' */
    Command del = {CMD_DELETE_CHAR, 0};
    editor_execute(&ed, del);          /* buffer -> "ac" */
    ASSERT_EQ(ed.buffer->lines[0].len, 2);
    ASSERT(ed.buffer->lines[0].chars[1] == 'c');

    Command undo = {CMD_UNDO, 0};
    editor_execute(&ed, undo);
    ASSERT_EQ(ed.buffer->lines[0].len, 3);
    ASSERT(ed.buffer->lines[0].chars[0] == 'a');
    ASSERT(ed.buffer->lines[0].chars[1] == 'b');
    ASSERT(ed.buffer->lines[0].chars[2] == 'c');

    test_editor_free(&ed);
}

TEST(undo_backspace_restores_char)
{
    Editor ed;
    test_editor_init(&ed);

    type_str(&ed, "abc");              /* cursor at col 3 */
    Command bs = {CMD_BACKSPACE, 0};
    editor_execute(&ed, bs);           /* delete 'c' -> "ab" */
    ASSERT_EQ(ed.buffer->lines[0].len, 2);
    ASSERT_EQ(ed.cursor.col, 2);

    Command undo = {CMD_UNDO, 0};
    editor_execute(&ed, undo);
    ASSERT_EQ(ed.buffer->lines[0].len, 3);
    ASSERT(ed.buffer->lines[0].chars[2] == 'c');
    ASSERT_EQ(ed.cursor.col, 3);

    test_editor_free(&ed);
}

TEST(undo_newline_merges_lines_back)
{
    /* splitting "abcd" at col 2 then undoing should restore one line */
    Editor ed;
    test_editor_init(&ed);

    type_str(&ed, "abcd");
    ed.cursor.col = 2;
    Command nl = {CMD_INSERT_NEWLINE, 0};
    editor_execute(&ed, nl);
    ASSERT_EQ(ed.buffer->num_lines, 2);
    ASSERT_EQ(ed.cursor.row, 1);
    ASSERT_EQ(ed.cursor.col, 0);

    Command undo = {CMD_UNDO, 0};
    editor_execute(&ed, undo);
    ASSERT_EQ(ed.buffer->num_lines, 1);
    ASSERT_EQ(ed.buffer->lines[0].len, 4);
    ASSERT(ed.buffer->lines[0].chars[2] == 'c');
    /* cursor returns to the split point */
    ASSERT_EQ(ed.cursor.row, 0);
    ASSERT_EQ(ed.cursor.col, 2);

    test_editor_free(&ed);
}

TEST(redo_newline_resplits_lines)
{
    Editor ed;
    test_editor_init(&ed);

    type_str(&ed, "abcd");
    ed.cursor.col = 2;
    Command nl = {CMD_INSERT_NEWLINE, 0};
    editor_execute(&ed, nl);

    Command undo = {CMD_UNDO, 0};
    Command redo = {CMD_REDO, 0};
    editor_execute(&ed, undo);
    ASSERT_EQ(ed.buffer->num_lines, 1);

    editor_execute(&ed, redo);
    ASSERT_EQ(ed.buffer->num_lines, 2);
    ASSERT_EQ(ed.buffer->lines[0].len, 2);
    ASSERT_EQ(ed.buffer->lines[1].len, 2);
    ASSERT(ed.buffer->lines[1].chars[0] == 'c');
    ASSERT_EQ(ed.cursor.row, 1);
    ASSERT_EQ(ed.cursor.col, 0);

    test_editor_free(&ed);
}

TEST(undo_backspace_at_col_zero_merges_and_reverses)
{
    /* backspace at the start of a line merges it with the previous line;
       undo must split it back into two lines as a single atomic step */
    Editor ed;
    test_editor_init(&ed);

    type_str(&ed, "ab");
    Command nl = {CMD_INSERT_NEWLINE, 0};
    editor_execute(&ed, nl);           /* line0 "ab", line1 "" */
    type_str(&ed, "cd");               /* line1 "cd" */
    ed.cursor.row = 1;
    ed.cursor.col = 0;

    Command bs = {CMD_BACKSPACE, 0};
    editor_execute(&ed, bs);           /* merge -> single line "abcd" */
    ASSERT_EQ(ed.buffer->num_lines, 1);
    ASSERT_EQ(ed.buffer->lines[0].len, 4);
    ASSERT_EQ(ed.cursor.row, 0);
    ASSERT_EQ(ed.cursor.col, 2);

    Command undo = {CMD_UNDO, 0};
    editor_execute(&ed, undo);         /* split back into two lines */
    ASSERT_EQ(ed.buffer->num_lines, 2);
    ASSERT_EQ(ed.buffer->lines[0].len, 2);
    ASSERT_EQ(ed.buffer->lines[1].len, 2);
    ASSERT(ed.buffer->lines[1].chars[0] == 'c');
    /* cursor returns to the start of the previously-merged line */
    ASSERT_EQ(ed.cursor.row, 1);
    ASSERT_EQ(ed.cursor.col, 0);

    test_editor_free(&ed);
}

TEST(new_edit_clears_redo_via_editor)
{
    /* undo then a new edit should make redo a no-op */
    Editor ed;
    test_editor_init(&ed);

    type_str(&ed, "ab");
    Command undo = {CMD_UNDO, 0};
    Command redo = {CMD_REDO, 0};
    editor_execute(&ed, undo);         /* remove 'b', buffer "a" */
    ASSERT_EQ(ed.buffer->lines[0].len, 1);

    type_str(&ed, "x");                /* fresh edit clears redo */
    ASSERT_EQ(ed.buffer->lines[0].len, 2);
    ASSERT(ed.buffer->lines[0].chars[1] == 'x');

    editor_execute(&ed, redo);         /* should do nothing */
    ASSERT_EQ(ed.buffer->lines[0].len, 2);
    ASSERT(ed.buffer->lines[0].chars[1] == 'x');

    test_editor_free(&ed);
}

TEST(undo_redo_full_roundtrip)
{
    /* a sequence of mixed edits should undo back to empty and redo back to
       the exact same final buffer state */
    Editor ed;
    test_editor_init(&ed);

    type_str(&ed, "hello");
    Command nl = {CMD_INSERT_NEWLINE, 0};
    editor_execute(&ed, nl);
    type_str(&ed, "world");
    /* state: line0 "hello", line1 "world" */

    Command undo = {CMD_UNDO, 0};
    /* 5 (world) + 1 (newline) + 5 (hello) = 11 edits to fully undo */
    for (int i = 0; i < 11; i++)
    {
        editor_execute(&ed, undo);
    }
    ASSERT_EQ(ed.buffer->num_lines, 1);
    ASSERT_EQ(ed.buffer->lines[0].len, 0);

    Command redo = {CMD_REDO, 0};
    for (int i = 0; i < 11; i++)
    {
        editor_execute(&ed, redo);
    }
    ASSERT_EQ(ed.buffer->num_lines, 2);
    ASSERT_EQ(ed.buffer->lines[0].len, 5);
    ASSERT_EQ(ed.buffer->lines[1].len, 5);
    ASSERT(ed.buffer->lines[0].chars[0] == 'h');
    ASSERT(ed.buffer->lines[1].chars[0] == 'w');

    test_editor_free(&ed);
}
