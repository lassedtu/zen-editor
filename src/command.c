#include "command.h"

#include "buffer.h"
#include "cursor.h"
#include "undo.h"

/**
 * @file command.c
 * @brief command execution logic for the editor.
 *
 * this file implements editor_execute(), which maps each CommandType to the
 * corresponding mutation of editor state. all editing actions flow through
 * this single function, which is also where edits are recorded into the undo
 * history so they can later be reversed or replayed.
 */

/**
 * @brief build a single-operation undo entry.
 * @param op the inverse operation that reverses the edit.
 * @param cur_row cursor row to restore when the edit is undone (pre-edit).
 * @param cur_col cursor col to restore when the edit is undone (pre-edit).
 * @param redo_row cursor row to restore when the edit is redone (post-edit).
 * @param redo_col cursor col to restore when the edit is redone (post-edit).
 * @return the assembled UndoEntry.
 */
static UndoEntry make_entry(UndoOp op, int cur_row, int cur_col,
                            int redo_row, int redo_col)
{
    UndoEntry entry;
    entry.num_ops = 1;
    entry.ops[0] = op;
    entry.cursor_row = cur_row;
    entry.cursor_col = cur_col;
    entry.redo_row = redo_row;
    entry.redo_col = redo_col;
    return entry;
}

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
    {
        int row = ed->cursor.row;
        int col = ed->cursor.col;
        buffer_insert_char(ed->buffer, row, col, (char)cmd.ch);
        ed->cursor.col++;

        /* undo by deleting the character we just inserted */
        UndoOp op = {UNDO_OP_DELETE_CHAR, row, col, 0};
        history_record(&ed->history,
                       make_entry(op, row, col, row, col + 1));
        break;
    }

    case CMD_DELETE_CHAR:
    {
        int row = ed->cursor.row;
        int col = ed->cursor.col;
        Line *line = &ed->buffer->lines[row];
        /* only record if there is actually a character to delete here */
        if (col >= 0 && col < line->len)
        {
            char deleted = line->chars[col];
            buffer_delete_char(ed->buffer, row, col);

            /* undo by reinserting the deleted character; cursor stays put */
            UndoOp op = {UNDO_OP_INSERT_CHAR, row, col, deleted};
            history_record(&ed->history,
                           make_entry(op, row, col, row, col));
        }
        break;
    }

    case CMD_BACKSPACE:
        if (ed->cursor.col > 0)
        {
            int row = ed->cursor.row;
            int col = ed->cursor.col;
            char deleted = ed->buffer->lines[row].chars[col - 1];
            ed->cursor.col--;
            buffer_delete_char(ed->buffer, row, col - 1);

            /* undo by reinserting the deleted character before the cursor */
            UndoOp op = {UNDO_OP_INSERT_CHAR, row, col - 1, deleted};
            history_record(&ed->history,
                           make_entry(op, row, col, row, col - 1));
        }
        else if (ed->cursor.row > 0)
        {
            int row = ed->cursor.row;
            int prev_len = ed->buffer->lines[row - 1].len;
            buffer_delete_line(ed->buffer, row);
            ed->cursor.row--;
            ed->cursor.col = prev_len;

            /* undo by splitting the merged line back at the join column */
            UndoOp op = {UNDO_OP_SPLIT_LINE, row - 1, prev_len, 0};
            history_record(&ed->history,
                           make_entry(op, row, 0, row - 1, prev_len));
        }
        break;

    case CMD_INSERT_NEWLINE:
    {
        int row = ed->cursor.row;
        int col = ed->cursor.col;
        buffer_insert_newline(ed->buffer, row, col);
        ed->cursor.row++;
        ed->cursor.col = 0;

        /* undo by merging the newly created line back into this one */
        UndoOp op = {UNDO_OP_MERGE_LINE, row + 1, 0, 0};
        history_record(&ed->history,
                       make_entry(op, row, col, row + 1, 0));
        break;
    }

    case CMD_UNDO:
    {
        int r, c;
        if (history_undo(&ed->history, ed->buffer, &r, &c))
        {
            ed->cursor.row = r;
            ed->cursor.col = c;
            cursor_clamp(&ed->cursor, ed->buffer);
        }
        break;
    }

    case CMD_REDO:
    {
        int r, c;
        if (history_redo(&ed->history, ed->buffer, &r, &c))
        {
            ed->cursor.row = r;
            ed->cursor.col = c;
            cursor_clamp(&ed->cursor, ed->buffer);
        }
        break;
    }

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
