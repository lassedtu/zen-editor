/**
 * @file test_config.c
 * @brief unit tests for the configuration module and its integration.
 *
 * the tests cover three layers. the first layer is the Config module alone: the
 * defaults, the key lookup, and the parse of a file with settings and bindings.
 * the second layer is the tab-size visual column in the renderer. the third
 * layer is the auto-save count through editor_execute.
 */

#include "config.h"
#include "keymap.h"
#include "renderer.h"
#include "command.h"
#include "editor.h"
#include "keys.h"
#include "buffer.h"
#include "undo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief write text to a temporary file and return its path.
 *
 * the function writes the text to a fixed path in the temporary area. the test
 * removes the file when it is done.
 *
 * @param text the text to write.
 * @param path a buffer that receives the path; must hold at least 64 bytes.
 */
static void cfg_write_temp(const char *text, char *path)
{
    strcpy(path, "build/test_zerc.tmp");
    FILE *f = fopen(path, "w");
    if (f != NULL)
    {
        fwrite(text, 1, strlen(text), f);
        fclose(f);
    }
}

/**
 * @brief build a minimal Editor around a fresh buffer without the terminal.
 * @param ed pointer to the Editor to initialize.
 */
static void cfg_editor_init(Editor *ed)
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
    config_init(&ed->config);
    ed->edits_since_save = 0;
}

/**
 * @brief release everything owned by a test editor.
 * @param ed pointer to the Editor to tear down.
 */
static void cfg_editor_free(Editor *ed)
{
    buffer_free(ed->buffer);
    history_free(&ed->history);
    clipboard_free(&ed->clipboard);
    free(ed->filename);
}

TEST(config_defaults_are_sensible)
{
    Config cfg;
    config_init(&cfg);
    ASSERT_EQ(cfg.tab_size, 4);
    ASSERT_EQ(cfg.auto_save_edits, 0);
    /* the theme uses the terminal colors by default */
    ASSERT_STR_EQ(cfg.theme.foreground, "");
    ASSERT_STR_EQ(cfg.theme.status_bar, "");
    /* the default bindings include the built-in control keys */
    ASSERT_EQ(config_command_for_key(&cfg, KEY_CTRL('q')), CMD_QUIT);
    ASSERT_EQ(config_command_for_key(&cfg, KEY_CTRL('s')), CMD_SAVE);
}

TEST(config_command_for_unbound_key_is_none)
{
    Config cfg;
    config_init(&cfg);
    ASSERT_EQ(config_command_for_key(&cfg, KEY_CTRL('w')), CMD_NONE);
}

TEST(config_parses_tab_size)
{
    Config cfg;
    config_init(&cfg);
    char path[64];
    cfg_write_temp("tab_size = 8\n", path);
    ASSERT_EQ(config_load_file(&cfg, path), 0);
    ASSERT_EQ(cfg.tab_size, 8);
    remove(path);
}

TEST(config_rejects_out_of_range_tab_size)
{
    /* a value outside 1..16 must keep the current tab size */
    Config cfg;
    config_init(&cfg);
    char path[64];
    cfg_write_temp("tab_size = 999\n", path);
    config_load_file(&cfg, path);
    ASSERT_EQ(cfg.tab_size, 4);
    remove(path);
}

TEST(config_parses_auto_save)
{
    Config cfg;
    config_init(&cfg);
    char path[64];
    cfg_write_temp("auto_save = 5\n", path);
    config_load_file(&cfg, path);
    ASSERT_EQ(cfg.auto_save_edits, 5);
    remove(path);
}

TEST(config_parses_theme_colors)
{
    Config cfg;
    config_init(&cfg);
    char path[64];
    cfg_write_temp("color_foreground = 32\ncolor_status_bar = 44\n", path);
    config_load_file(&cfg, path);
    ASSERT_STR_EQ(cfg.theme.foreground, "32");
    ASSERT_STR_EQ(cfg.theme.status_bar, "44");
    remove(path);
}

TEST(config_ignores_comments_and_blank_lines)
{
    Config cfg;
    config_init(&cfg);
    char path[64];
    cfg_write_temp("# a comment\n\n   \ntab_size = 2\n", path);
    config_load_file(&cfg, path);
    ASSERT_EQ(cfg.tab_size, 2);
    remove(path);
}

TEST(config_ignores_unknown_key)
{
    /* an unknown key must not change any setting */
    Config cfg;
    config_init(&cfg);
    char path[64];
    cfg_write_temp("unknown_key = 123\ntab_size = 3\n", path);
    config_load_file(&cfg, path);
    ASSERT_EQ(cfg.tab_size, 3);
    remove(path);
}

TEST(config_parses_key_binding)
{
    Config cfg;
    config_init(&cfg);
    char path[64];
    cfg_write_temp("bind ctrl+w = quit\n", path);
    config_load_file(&cfg, path);
    ASSERT_EQ(config_command_for_key(&cfg, KEY_CTRL('w')), CMD_QUIT);
    remove(path);
}

TEST(config_binding_can_override_default)
{
    /* rebind ctrl+q to save; the key must now run save, not quit */
    Config cfg;
    config_init(&cfg);
    char path[64];
    cfg_write_temp("bind ctrl+q = save\n", path);
    config_load_file(&cfg, path);
    ASSERT_EQ(config_command_for_key(&cfg, KEY_CTRL('q')), CMD_SAVE);
    remove(path);
}

TEST(config_binding_with_unknown_command_is_ignored)
{
    Config cfg;
    config_init(&cfg);
    char path[64];
    cfg_write_temp("bind ctrl+w = fly\n", path);
    config_load_file(&cfg, path);
    ASSERT_EQ(config_command_for_key(&cfg, KEY_CTRL('w')), CMD_NONE);
    remove(path);
}

TEST(config_load_missing_file_returns_error)
{
    Config cfg;
    config_init(&cfg);
    ASSERT_EQ(config_load_file(&cfg, "build/does_not_exist.tmp"), -1);
    /* the defaults must stay in place after a missing file */
    ASSERT_EQ(cfg.tab_size, 4);
}

TEST(keymap_config_uses_binding_before_builtin)
{
    /* a rebound key must beat the built-in map */
    Config cfg;
    config_init(&cfg);
    char path[64];
    cfg_write_temp("bind ctrl+w = quit\n", path);
    config_load_file(&cfg, path);
    Command cmd = keymap_translate_config(&cfg, KEY_CTRL('w'));
    ASSERT_EQ(cmd.type, CMD_QUIT);
    remove(path);
}

TEST(keymap_config_falls_back_to_builtin)
{
    /* a key with no binding must still use the built-in map */
    Config cfg;
    config_init(&cfg);
    Command cmd = keymap_translate_config(&cfg, KEY_ARROW_UP);
    ASSERT_EQ(cmd.type, CMD_MOVE_UP);
}

TEST(renderer_visual_col_expands_tabs)
{
    /* a line "\tab" with tab size 4: column 1 is visual column 4 */
    Buffer *buf = buffer_create();
    buffer_insert_char(buf, 0, 0, '\t');
    buffer_insert_char(buf, 0, 1, 'a');
    buffer_insert_char(buf, 0, 2, 'b');
    ASSERT_EQ(renderer_visual_col(&buf->lines[0], 0, 4), 0);
    ASSERT_EQ(renderer_visual_col(&buf->lines[0], 1, 4), 4);
    ASSERT_EQ(renderer_visual_col(&buf->lines[0], 2, 4), 5);
    buffer_free(buf);
}

TEST(renderer_visual_col_partial_tab_stop)
{
    /* "ab\t" with tab size 4: after "ab" the tab fills to column 4 */
    Buffer *buf = buffer_create();
    buffer_insert_char(buf, 0, 0, 'a');
    buffer_insert_char(buf, 0, 1, 'b');
    buffer_insert_char(buf, 0, 2, '\t');
    ASSERT_EQ(renderer_visual_col(&buf->lines[0], 2, 4), 2);
    ASSERT_EQ(renderer_visual_col(&buf->lines[0], 3, 4), 4);
    buffer_free(buf);
}

TEST(auto_save_triggers_after_edit_count)
{
    /* with auto_save = 2, the third edit removes the counter reset; the file is
       written after the count reaches the limit. this test checks the counter,
       because a real file write goes through the platform layer. */
    Editor ed;
    cfg_editor_init(&ed);
    ed.config.auto_save_edits = 2;
    ed.filename = malloc(32);
    strcpy(ed.filename, "build/test_autosave.tmp");

    Command type_a = {CMD_INSERT_CHAR, 'a'};
    editor_execute(&ed, type_a);
    ASSERT_EQ(ed.edits_since_save, 1);

    Command type_b = {CMD_INSERT_CHAR, 'b'};
    editor_execute(&ed, type_b);
    /* the count reached 2, so auto-save ran and set the count back to 0 */
    ASSERT_EQ(ed.edits_since_save, 0);

    remove("build/test_autosave.tmp");
    cfg_editor_free(&ed);
}

TEST(auto_save_off_by_default_keeps_counting)
{
    /* with auto_save = 0, the counter grows and never resets by itself */
    Editor ed;
    cfg_editor_init(&ed);
    ed.filename = malloc(32);
    strcpy(ed.filename, "build/test_autosave.tmp");

    Command type_a = {CMD_INSERT_CHAR, 'a'};
    editor_execute(&ed, type_a);
    editor_execute(&ed, type_a);
    editor_execute(&ed, type_a);
    ASSERT_EQ(ed.edits_since_save, 3);

    cfg_editor_free(&ed);
}

TEST(manual_save_resets_edit_count)
{
    Editor ed;
    cfg_editor_init(&ed);
    ed.filename = malloc(32);
    strcpy(ed.filename, "build/test_autosave.tmp");

    Command type_a = {CMD_INSERT_CHAR, 'a'};
    editor_execute(&ed, type_a);
    ASSERT_EQ(ed.edits_since_save, 1);

    Command save = {CMD_SAVE, 0};
    editor_execute(&ed, save);
    ASSERT_EQ(ed.edits_since_save, 0);

    remove("build/test_autosave.tmp");
    cfg_editor_free(&ed);
}

TEST(cursor_move_does_not_count_as_edit)
{
    Editor ed;
    cfg_editor_init(&ed);
    buffer_insert_char(ed.buffer, 0, 0, 'x');
    ed.edits_since_save = 0;

    Command move = {CMD_MOVE_LEFT, 0};
    editor_execute(&ed, move);
    ASSERT_EQ(ed.edits_since_save, 0);

    cfg_editor_free(&ed);
}
