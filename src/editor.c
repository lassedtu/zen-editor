#include "editor.h"
#include "renderer.h"
#include "platform_terminal.h"
#include <stdlib.h>
#include <string.h>

// macro to convert control characters to their corresponding key codes
#define KEY_CTRL(k) ((k) & 0x1f)

/**
 * @enum EditorKey
 * @brief special key codes for the editor.
 */
enum EditorKey
{
    KEY_ARROW_UP = 1000, // arbitrary starting value to avoid conflict with ASCII
    KEY_ARROW_DOWN,
    KEY_ARROW_LEFT,
    KEY_ARROW_RIGHT,
    KEY_HOME,
    KEY_END,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,
    KEY_DELETE
};

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
 * @brief process a single key press and update the editor state accordingly
 * @param ed pointer to the editor state
 */
static void editor_process_key(Editor *ed)
{
    int key = platform_terminal_read_key();

    switch (key)
    {
    case KEY_CTRL('q'):
        ed->running = 0;
        break;

    case KEY_CTRL('s'):
        if (ed->filename)
        {
            buffer_save(ed->buffer, ed->filename);
        }
        break;

    case KEY_ARROW_UP:
        cursor_move_up(&ed->cursor, ed->buffer);
        break;
    case KEY_ARROW_DOWN:
        cursor_move_down(&ed->cursor, ed->buffer);
        break;
    case KEY_ARROW_LEFT:
        cursor_move_left(&ed->cursor, ed->buffer);
        break;
    case KEY_ARROW_RIGHT:
        cursor_move_right(&ed->cursor, ed->buffer);
        break;

    case KEY_HOME:
        cursor_home(&ed->cursor);
        break;
    case KEY_END:
        cursor_end(&ed->cursor, ed->buffer);
        break;

    case KEY_DELETE:
        buffer_delete_char(ed->buffer, ed->cursor.row, ed->cursor.col);
        break;

    case 127: /* backspace */
        if (ed->cursor.col > 0)
        {
            ed->cursor.col--;
            buffer_delete_char(ed->buffer, ed->cursor.row, ed->cursor.col);
        }
        else if (ed->cursor.row > 0)
        {
            int prev_len = ed->buffer->lines[ed->cursor.row - 1].len;
            buffer_delete_line(ed->buffer, ed->cursor.row);
            ed->cursor.row--;
            ed->cursor.col = prev_len;
        }
        break;

    case '\r': /* enter */
        buffer_insert_newline(ed->buffer, ed->cursor.row, ed->cursor.col);
        ed->cursor.row++;
        ed->cursor.col = 0;
        break;

    default:
        /* insert printable characters */
        if (key >= 32 && key < 127)
        {
            buffer_insert_char(ed->buffer, ed->cursor.row, ed->cursor.col,
                               (char)key);
            ed->cursor.col++;
        }
        break;
    }
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
