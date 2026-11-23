#ifndef ZE_COMMAND_H
#define ZE_COMMAND_H

/**
 * @file command.h
 * @brief command types and execution for the editor.
 *
 * this file defines the command abstraction layer. every editor action is
 * represented as a Command, enabling decoupled key bindings, future undo/redo
 * recording, and macro support.
 */

/* forward declaration: the command layer only needs a pointer to the Editor,
   so it does not include editor.h. this also stops an include cycle, because
   editor.h reads config.h and config.h reads this header. */
struct Editor;
typedef struct Editor Editor;

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
    CMD_MOVE_WORD_LEFT,
    CMD_MOVE_WORD_RIGHT,
    CMD_HOME,
    CMD_END,
    CMD_PAGE_UP,
    CMD_PAGE_DOWN,
    CMD_INSERT_CHAR,
    CMD_DELETE_CHAR,
    CMD_BACKSPACE,
    CMD_INSERT_NEWLINE,
    CMD_UNDO,
    CMD_REDO,
    CMD_SEARCH_OPEN,
    CMD_SEARCH_NEXT,
    CMD_SEARCH_PREV,
    CMD_SEARCH_CLOSE,
    CMD_SELECT_LEFT,
    CMD_SELECT_RIGHT,
    CMD_SELECT_UP,
    CMD_SELECT_DOWN,
    CMD_SELECT_HOME,
    CMD_SELECT_END,
    CMD_SELECT_WORD_LEFT,
    CMD_SELECT_WORD_RIGHT,
    CMD_SELECT_ALL,
    CMD_COPY,
    CMD_CUT,
    CMD_PASTE,
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
