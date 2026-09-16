#include "editor.h"
#include "command.h"
#include "keymap.h"
#include "keys.h"
#include "renderer.h"
#include "search.h"
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
    history_init(&ed->history);

    ed->search.active = 0;
    ed->search.query[0] = '\0';
    ed->search.query_len = 0;

    selection_clear(&ed->selection);

    if (platform_terminal_init() != 0)
    {
        buffer_free(ed->buffer);
        return -1;
    }

    platform_terminal_get_size(&ed->screen_rows, &ed->screen_cols);

    if (filename)
    {
        ed->filename = malloc(strlen(filename) + 1);
        if (ed->filename != NULL) {
            strcpy(ed->filename, filename);
        }
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
 * @brief handle a key while the search prompt is active.
 *
 * captures input for the query directly instead of routing through the keymap:
 * printable characters extend the query, Backspace shortens it, Enter and the
 * next/prev keys step between matches, and Escape cancels back to the origin.
 *
 * @param ed pointer to the editor state.
 * @param key the raw key code from the platform layer.
 */
static void editor_process_search_key(Editor *ed, int key)
{
    switch (key)
    {
    case KEY_ESCAPE:
        editor_search_cancel(ed);
        break;

    case KEY_ENTER:
        editor_search_close(ed);
        break;

    case KEY_BACKSPACE:
        editor_search_backspace(ed);
        break;

    case KEY_ARROW_DOWN:
    case KEY_CTRL('n'):
        editor_search_next(ed);
        break;

    case KEY_ARROW_UP:
    case KEY_CTRL('p'):
        editor_search_prev(ed);
        break;

    default:
        if (key >= 32 && key < 127)
        {
            editor_search_input_char(ed, (char)key);
        }
        break;
    }
}

/**
 * @brief read a key and execute the corresponding command.
 * @param ed pointer to the editor state.
 */
static void editor_process_key(Editor *ed)
{
    int key = platform_terminal_read_key();

    if (ed->search.active)
    {
        editor_process_search_key(ed, key);
        return;
    }

    Command cmd = keymap_translate(key);
    editor_execute(ed, cmd);
}

void editor_run(Editor *ed)
{
    while (ed->running)
    {
        /* check if the terminal was resized and update dimensions */
        if (platform_terminal_has_resized())
        {
            platform_terminal_get_size(&ed->screen_rows, &ed->screen_cols);
        }

        editor_scroll(ed); // ensure cursor is visible

        /* highlight matches only while the search prompt is open */
        const char *highlight = ed->search.active ? ed->search.query : NULL;

        // draw the buffer and cursor
        renderer_draw(ed->buffer, &ed->cursor, ed->screen_rows,
                      ed->screen_cols, ed->scroll_offset, highlight,
                      ed->selection.active ? &ed->selection : NULL);

        if (ed->search.active)
        {
            /* count matches across the whole buffer for the prompt readout */
            int match_count = 0;
            for (int i = 0; i < ed->buffer->num_lines; i++)
            {
                match_count += search_count_in_line(&ed->buffer->lines[i],
                                                     ed->search.query);
            }
            renderer_draw_search_prompt(ed->search.query, match_count,
                                        ed->screen_rows, ed->screen_cols);
        }
        else
        {
            // draw the status bar
            renderer_draw_status(ed->filename, ed->buffer->num_lines,
                                 ed->cursor.row, ed->screen_rows);
        }

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
    history_free(&ed->history);
    free(ed->filename);
}
