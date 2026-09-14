#include "keymap.h"
#include "keys.h"

/**
 * @file keymap.c
 * @brief key-to-command translation implementation.
 *
 * this file maps raw key codes to Commands. the current mapping is hard-coded
 * but isolated here so that future configurable bindings only need to change
 * this file.
 *
 * platform note (macOS): the Command key does not reach a terminal program.
 * the terminal emulator handles the Command key itself and does not send a
 * byte for it. therefore the editor cannot bind Command+C, Command+V,
 * Command+Z, or Command+Y from inside the program. the editor binds the
 * portable Control equivalents, which the terminal does deliver. to get
 * Command shortcuts on macOS, map them in the terminal emulator so that each
 * Command combination sends the matching Control byte (for example, map
 * Command+C to send Control+C). the editor then receives the Control byte and
 * runs the command.
 *
 * quit note (macOS): Command+Q closes the terminal window and cannot be
 * remapped safely, so the editor keeps Control+Q as the quit key. do not map
 * Command+Q to quit the editor.
 */

Command keymap_translate(int key)
{
    Command cmd = {CMD_NONE, 0};

    switch (key)
    {
    case KEY_CTRL('q'):
        cmd.type = CMD_QUIT;
        break;

    case KEY_CTRL('s'):
        cmd.type = CMD_SAVE;
        break;

    case KEY_CTRL('z'):
        cmd.type = CMD_UNDO;
        break;

    case KEY_CTRL('y'):
        cmd.type = CMD_REDO;
        break;

    case KEY_CTRL('f'):
        cmd.type = CMD_SEARCH_OPEN;
        break;

    case KEY_CTRL('a'):
        cmd.type = CMD_SELECT_ALL;
        break;

    case KEY_CTRL('c'):
        cmd.type = CMD_COPY;
        break;

    case KEY_CTRL('x'):
        cmd.type = CMD_CUT;
        break;

    case KEY_CTRL('v'):
        cmd.type = CMD_PASTE;
        break;

    case KEY_SHIFT_ARROW_UP:
        cmd.type = CMD_SELECT_UP;
        break;

    case KEY_SHIFT_ARROW_DOWN:
        cmd.type = CMD_SELECT_DOWN;
        break;

    case KEY_SHIFT_ARROW_LEFT:
        cmd.type = CMD_SELECT_LEFT;
        break;

    case KEY_SHIFT_ARROW_RIGHT:
        cmd.type = CMD_SELECT_RIGHT;
        break;

    case KEY_SHIFT_HOME:
        cmd.type = CMD_SELECT_HOME;
        break;

    case KEY_SHIFT_END:
        cmd.type = CMD_SELECT_END;
        break;

    case KEY_ALT_ARROW_LEFT:
        cmd.type = CMD_MOVE_WORD_LEFT;
        break;

    case KEY_ALT_ARROW_RIGHT:
        cmd.type = CMD_MOVE_WORD_RIGHT;
        break;

    case KEY_ALT_SHIFT_ARROW_LEFT:
        cmd.type = CMD_SELECT_WORD_LEFT;
        break;

    case KEY_ALT_SHIFT_ARROW_RIGHT:
        cmd.type = CMD_SELECT_WORD_RIGHT;
        break;

    case KEY_ARROW_UP:
        cmd.type = CMD_MOVE_UP;
        break;

    case KEY_ARROW_DOWN:
        cmd.type = CMD_MOVE_DOWN;
        break;

    case KEY_ARROW_LEFT:
        cmd.type = CMD_MOVE_LEFT;
        break;

    case KEY_ARROW_RIGHT:
        cmd.type = CMD_MOVE_RIGHT;
        break;

    case KEY_HOME:
        cmd.type = CMD_HOME;
        break;

    case KEY_END:
        cmd.type = CMD_END;
        break;

    case KEY_PAGE_UP:
        cmd.type = CMD_PAGE_UP;
        break;

    case KEY_PAGE_DOWN:
        cmd.type = CMD_PAGE_DOWN;
        break;

    case KEY_DELETE:
        cmd.type = CMD_DELETE_CHAR;
        break;

    case KEY_BACKSPACE:
        cmd.type = CMD_BACKSPACE;
        break;

    case KEY_ENTER:
        cmd.type = CMD_INSERT_NEWLINE;
        break;

    default:
        if (key >= 32 && key < 127)
        {
            cmd.type = CMD_INSERT_CHAR;
            cmd.ch = key;
        }
        break;
    }

    return cmd;
}
