#ifndef ZE_EDITOR_H
#define ZE_EDITOR_H

#include "buffer.h"
#include "cursor.h"

/**
 * @file editor.h
 * @brief main editor state and functions.
 *
 * this file defines the Editor structure, which holds the state of the text editor, including the buffer, cursor position, screen dimensions, and other relevant information. It also declares functions for initializing, running, and cleaning up the editor.
 */

/**
 * @struct Editor
 * @brief represents the state of the text editor.
 */
typedef struct
{
    Buffer *buffer;    // pointer to the text buffer
    Cursor cursor;     // current position of the cursor in the buffer
    int screen_rows;   // number of rows in the terminal screen
    int screen_cols;   // number of columns in the terminal screen
    int scroll_offset; // vertical scroll offset of the editor
    int running;       // flag indicating if the editor is running
    char *filename;    // name of the currently opened file
} Editor;

/**
 * @brief initialize the editor state, including buffer, cursor, and terminal.
 * @param ed pointer to the editor state
 * @param filename optional filename to load into the buffer
 * @return 0 on success, -1 on failure
 */
int editor_init(Editor *ed, const char *filename);

/**
 * @brief run the main editor loop, handling input and rendering
 * @param ed pointer to the editor state
 */
void editor_run(Editor *ed);

/**
 * @brief clean up the editor state, freeing resources and restoring terminal settings
 * @param ed pointer to the editor state
 */
void editor_cleanup(Editor *ed);

#endif /* ZE_EDITOR_H */
