#ifndef ZE_COMMAND_H
#define ZE_COMMAND_H

#include "editor.h"

/**
 * @file command.h
 * @brief command types and execution for the editor.
 *
 * this file defines the command abstraction layer. every editor action is
 * represented as a Command, enabling decoupled key bindings, future undo/redo
 * recording, and macro support.
 */

/**
 * @enum CommandType
 * @brief identifies the action a command performs.
 */
typedef enum
{
    CMD_NONE,
    CMD_MOVE_UP,
    CMD_MOVE_DOWN,
    CMD_MOVE_LEFT,
    CMD_MOVE_RIGHT,
    CMD_HOME,
    CMD_END,
    CMD_PAGE_UP,
    CMD_PAGE_DOWN,
    CMD_INSERT_CHAR,
    CMD_DELETE_CHAR,
    CMD_BACKSPACE,
    CMD_INSERT_NEWLINE,
    CMD_SAVE,
    CMD_QUIT,
} CommandType;

/**
 * @struct Command
 * @brief represents a single editor command with optional payload.
 */
typedef struct
{
    CommandType type; // the action to perform
    int ch;           // character payload (used by CMD_INSERT_CHAR)
} Command;

/**
 * @brief execute a command against the editor state.
 * @param ed pointer to the editor state.
 * @param cmd the command to execute.
 */
void editor_execute(Editor *ed, Command cmd);

#endif /* ZE_COMMAND_H */
