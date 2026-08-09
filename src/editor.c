#include "editor.h"
#include "command.h"
#include "keymap.h"
#include "renderer.h"
#include "platform_terminal.h"
#include <stdlib.h>
#include <string.h>

int editor_init(Editor *ed, const char *filename)
{
    ed->buffer = buffer_create();
    if (!ed->buffer)
        return -1;

    ed->cursor.row = 0;
    ed->cursor.col = 0;
    ed->scroll_offset = 0;
    ed->running = 1;
    ed->filename = NULL;

    if (platform_terminal_init() != 0)
    {
        buffer_free(ed->buffer);
        return -1;
    }

    platform_terminal_get_size(&ed->screen_rows, &ed->screen_cols);

    if (filename)
    {
        ed->filename = strdup(filename);
        buffer_load(ed->buffer, filename);
    }

    return 0;
}

/**
 * @brief scroll the editor view to ensure the cursor is visible
 * @param ed pointer to the editor state
 */
static void editor_scroll(Editor *ed)
{
    int draw_rows = ed->screen_rows - 1;

    if (ed->cursor.row < ed->scroll_offset)
    {
        ed->scroll_offset = ed->cursor.row;
    }
    if (ed->cursor.row >= ed->scroll_offset + draw_rows)
    {
        ed->scroll_offset = ed->cursor.row - draw_rows + 1;
    }
}

/**
 * @brief read a key and execute the corresponding command.
 * @param ed pointer to the editor state.
 */
static void editor_process_key(Editor *ed)
{
    int key = platform_terminal_read_key();
    Command cmd = keymap_translate(key);
    editor_execute(ed, cmd);
}

void editor_run(Editor *ed)
{
    while (ed->running)
    {
        editor_scroll(ed); // ensure cursor is visible

        // draw the buffer and cursor
        renderer_draw(ed->buffer, &ed->cursor, ed->screen_rows,
                      ed->screen_cols, ed->scroll_offset);

        // draw the status bar
        renderer_draw_status(ed->filename, ed->buffer->num_lines,
                             ed->cursor.row, ed->screen_rows);

        // reposition cursor after drawing status
        platform_terminal_move_cursor(ed->cursor.row - ed->scroll_offset,
                                      ed->cursor.col);

        // flush the terminal output and process the next key press
        platform_terminal_flush();
        editor_process_key(ed);
    }
}

void editor_cleanup(Editor *ed)
{
    platform_terminal_cleanup();
    buffer_free(ed->buffer);
    free(ed->filename);
}
