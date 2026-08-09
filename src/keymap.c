#include "keymap.h"
#include "keys.h"

/**
 * @file keymap.c
 * @brief key-to-command translation implementation.
 *
 * this file maps raw key codes to Commands. the current mapping is hard-coded
 * but isolated here so that future configurable bindings only need to change
 * this file.
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
