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
    selection_clear(&ed->selection);
    clipboard_init(&ed->clipboard);
}

/**
 * @brief release everything owned by a test editor.
 * @param ed pointer to the Editor to tear down.
 */
static void test_editor_free(Editor *ed)
{
    buffer_free(ed->buffer);
    history_free(&ed->history);
    clipboard_free(&ed->clipboard);
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

    UndoOp op = {UNDO_OP_DELETE_CHAR, 0, 0, 0};
    history_record(&h, op, 0, 0, 0, 1);

    int r, c;
    history_undo(&h, buf, &r, &c); /* moves entry to redo stack */
    ASSERT_EQ(h.redo.count, 1);

    /* put the character back so the buffer state matches the op again, then a
       fresh record must clear the redo stack */
    buffer_insert_char(buf, 0, 0, 'a');
    history_record(&h, op, 0, 0, 0, 1); /* fresh edit clears redo */
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

    UndoOp op = {UNDO_OP_DELETE_CHAR, 0, 0, 0};
    for (int i = 0; i < UNDO_MAX_ENTRIES + 50; i++)
    {
        history_record(&h, op, 0, 0, 0, 0);
    }
    ASSERT_EQ(h.undo.count, UNDO_MAX_ENTRIES);

    history_free(&h);
    buffer_free(buf);
}

TEST(undo_insert_word_removes_whole_word)
{
    /* consecutive typed characters coalesce: one undo removes the whole word */
    Editor ed;
    test_editor_init(&ed);

    type_str(&ed, "hi");
    ASSERT_EQ(ed.buffer->lines[0].len, 2);
    ASSERT_EQ(ed.history.undo.count, 1); /* single coalesced entry */

    Command undo = {CMD_UNDO, 0};
    editor_execute(&ed, undo);
    ASSERT_EQ(ed.buffer->lines[0].len, 0);
    ASSERT_EQ(ed.cursor.col, 0);

    test_editor_free(&ed);
}

TEST(redo_reapplies_whole_word)
{
    /* undoing a coalesced word and redoing it restores the entire word */
    Editor ed;
    test_editor_init(&ed);

    type_str(&ed, "ab");
    Command undo = {CMD_UNDO, 0};
    Command redo = {CMD_REDO, 0};

    editor_execute(&ed, undo); /* remove the whole word "ab" */
    ASSERT_EQ(ed.buffer->lines[0].len, 0);

    editor_execute(&ed, redo); /* restore the whole word "ab" */
    ASSERT_EQ(ed.buffer->lines[0].len, 2);
    ASSERT(ed.buffer->lines[0].chars[0] == 'a');
    ASSERT(ed.buffer->lines[0].chars[1] == 'b');
    ASSERT_EQ(ed.cursor.col, 2);

    test_editor_free(&ed);
}

TEST(undo_delete_char_restores_it)
{
    Editor ed;
    test_editor_init(&ed);

    type_str(&ed, "abc");
    ed.cursor.col = 1; /* on 'b' */
    Command del = {CMD_DELETE_CHAR, 0};
    editor_execute(&ed, del); /* buffer -> "ac" */
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

    type_str(&ed, "abc"); /* cursor at col 3 */
    Command bs = {CMD_BACKSPACE, 0};
    editor_execute(&ed, bs); /* delete 'c' -> "ab" */
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
    editor_execute(&ed, nl); /* line0 "ab", line1 "" */
    type_str(&ed, "cd");     /* line1 "cd" */
    ed.cursor.row = 1;
    ed.cursor.col = 0;

    Command bs = {CMD_BACKSPACE, 0};
    editor_execute(&ed, bs); /* merge -> single line "abcd" */
    ASSERT_EQ(ed.buffer->num_lines, 1);
    ASSERT_EQ(ed.buffer->lines[0].len, 4);
    ASSERT_EQ(ed.cursor.row, 0);
    ASSERT_EQ(ed.cursor.col, 2);

    Command undo = {CMD_UNDO, 0};
    editor_execute(&ed, undo); /* split back into two lines */
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

    type_str(&ed, "ab"); /* one coalesced word */
    Command undo = {CMD_UNDO, 0};
    Command redo = {CMD_REDO, 0};
    editor_execute(&ed, undo); /* remove the whole word -> empty */
    ASSERT_EQ(ed.buffer->lines[0].len, 0);

    type_str(&ed, "x"); /* fresh edit clears redo */
    ASSERT_EQ(ed.buffer->lines[0].len, 1);
    ASSERT(ed.buffer->lines[0].chars[0] == 'x');

    editor_execute(&ed, redo); /* should do nothing */
    ASSERT_EQ(ed.buffer->lines[0].len, 1);
    ASSERT(ed.buffer->lines[0].chars[0] == 'x');

    test_editor_free(&ed);
}

TEST(undo_redo_full_roundtrip)
{
    /* a sequence of mixed edits should undo back to empty and redo back to
       the exact same final buffer state. with coalescing each typed word is a
       single step: "hello" + newline + "world" = 3 undo steps */
    Editor ed;
    test_editor_init(&ed);

    type_str(&ed, "hello");
    Command nl = {CMD_INSERT_NEWLINE, 0};
    editor_execute(&ed, nl);
    type_str(&ed, "world");
    /* state: line0 "hello", line1 "world" */
    ASSERT_EQ(ed.history.undo.count, 3);

    Command undo = {CMD_UNDO, 0};
    for (int i = 0; i < 3; i++)
    {
        editor_execute(&ed, undo);
    }
    ASSERT_EQ(ed.buffer->num_lines, 1);
    ASSERT_EQ(ed.buffer->lines[0].len, 0);

    Command redo = {CMD_REDO, 0};
    for (int i = 0; i < 3; i++)
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

TEST(coalesce_spaces_separate_words)
{
    /* "cat dog" is two words: one undo removes "dog", the next removes "cat "
       (the space stays attached to the first word) */
    Editor ed;
    test_editor_init(&ed);

    type_str(&ed, "cat dog");
    ASSERT_EQ(ed.history.undo.count, 2);

    Command undo = {CMD_UNDO, 0};
    editor_execute(&ed, undo);
    ASSERT_EQ(ed.buffer->lines[0].len, 4); /* "cat " remains */
    ASSERT(ed.buffer->lines[0].chars[3] == ' ');

    editor_execute(&ed, undo);
    ASSERT_EQ(ed.buffer->lines[0].len, 0);

    test_editor_free(&ed);
}

TEST(coalesce_multiple_spaces_group_with_word)
{
    /* runs of whitespace stay with the preceding word rather than forming
       their own undo steps: "a  b" -> two words ("a  " and "b") */
    Editor ed;
    test_editor_init(&ed);

    type_str(&ed, "a  b");
    ASSERT_EQ(ed.history.undo.count, 2);

    Command undo = {CMD_UNDO, 0};
    editor_execute(&ed, undo);
    ASSERT_EQ(ed.buffer->lines[0].len, 3); /* "a  " remains */

    test_editor_free(&ed);
}

TEST(coalesce_broken_by_delete)
{
    /* a non-typing edit ends the run so the next character starts a new word,
       even if it lands contiguously */
    Editor ed;
    test_editor_init(&ed);

    type_str(&ed, "ab"); /* run: "ab" */
    Command bs = {CMD_BACKSPACE, 0};
    editor_execute(&ed, bs); /* deletes 'b', ends the run */
    type_str(&ed, "c");      /* new run starts at col 1 */
    /* entries: [word "ab"], [backspace], [word "c"] */
    ASSERT_EQ(ed.history.undo.count, 3);

    Command undo = {CMD_UNDO, 0};
    editor_execute(&ed, undo); /* undo "c" */
    ASSERT_EQ(ed.buffer->lines[0].len, 1);
    ASSERT(ed.buffer->lines[0].chars[0] == 'a');

    test_editor_free(&ed);
}

TEST(coalesce_broken_across_lines)
{
    /* typing on a new line after Enter must not coalesce with the prior line */
    Editor ed;
    test_editor_init(&ed);

    type_str(&ed, "ab");
    Command nl = {CMD_INSERT_NEWLINE, 0};
    editor_execute(&ed, nl);
    type_str(&ed, "cd");
    /* entries: [word "ab"], [newline], [word "cd"] */
    ASSERT_EQ(ed.history.undo.count, 3);

    Command undo = {CMD_UNDO, 0};
    editor_execute(&ed, undo); /* removes only "cd" on line 1 */
    ASSERT_EQ(ed.buffer->num_lines, 2);
    ASSERT_EQ(ed.buffer->lines[1].len, 0);
    ASSERT_EQ(ed.buffer->lines[0].len, 2);

    test_editor_free(&ed);
}

TEST(coalesce_long_word_grows_ops_array)
{
    /* a word longer than the initial ops capacity must still undo as one step,
       exercising the growable ops array */
    Editor ed;
    test_editor_init(&ed);

    type_str(&ed, "supercalifragilistic"); /* 20 chars, one word */
    ASSERT_EQ(ed.history.undo.count, 1);
    ASSERT_EQ(ed.buffer->lines[0].len, 20);

    Command undo = {CMD_UNDO, 0};
    editor_execute(&ed, undo);
    ASSERT_EQ(ed.buffer->lines[0].len, 0);

    Command redo = {CMD_REDO, 0};
    editor_execute(&ed, redo);
    ASSERT_EQ(ed.buffer->lines[0].len, 20);
    ASSERT(ed.buffer->lines[0].chars[0] == 's');
    ASSERT(ed.buffer->lines[0].chars[19] == 'c');

    test_editor_free(&ed);
}
