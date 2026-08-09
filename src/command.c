#include "command.h"
#include "buffer.h"
#include "cursor.h"

/**
 * @file command.c
 * @brief command execution logic for the editor.
 *
 * this file implements editor_execute(), which maps each CommandType to the
 * corresponding mutation of editor state. all editing actions flow through
 * this single function, making it the future hook point for undo/redo recording.
 */

void editor_execute(Editor *ed, Command cmd)
{
    switch (cmd.type)
    {
    case CMD_NONE:
        break;

    case CMD_MOVE_UP:
        cursor_move_up(&ed->cursor, ed->buffer);
        break;

    case CMD_MOVE_DOWN:
        cursor_move_down(&ed->cursor, ed->buffer);
        break;

    case CMD_MOVE_LEFT:
        cursor_move_left(&ed->cursor, ed->buffer);
        break;

    case CMD_MOVE_RIGHT:
        cursor_move_right(&ed->cursor, ed->buffer);
        break;

    case CMD_HOME:
        cursor_home(&ed->cursor);
        break;

    case CMD_END:
        cursor_end(&ed->cursor, ed->buffer);
        break;

    case CMD_PAGE_UP:
    {
        int draw_rows = ed->screen_rows - 1;
        for (int i = 0; i < draw_rows; i++)
        {
            cursor_move_up(&ed->cursor, ed->buffer);
        }
        break;
    }

    case CMD_PAGE_DOWN:
    {
        int draw_rows = ed->screen_rows - 1;
        for (int i = 0; i < draw_rows; i++)
        {
            cursor_move_down(&ed->cursor, ed->buffer);
        }
        break;
    }

    case CMD_INSERT_CHAR:
        buffer_insert_char(ed->buffer, ed->cursor.row, ed->cursor.col,
                           (char)cmd.ch);
        ed->cursor.col++;
        break;

    case CMD_DELETE_CHAR:
        buffer_delete_char(ed->buffer, ed->cursor.row, ed->cursor.col);
        break;

    case CMD_BACKSPACE:
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

    case CMD_INSERT_NEWLINE:
        buffer_insert_newline(ed->buffer, ed->cursor.row, ed->cursor.col);
        ed->cursor.row++;
        ed->cursor.col = 0;
        break;

    case CMD_SAVE:
        if (ed->filename)
        {
            buffer_save(ed->buffer, ed->filename);
        }
        break;

    case CMD_QUIT:
        ed->running = 0;
        break;
    }
}
