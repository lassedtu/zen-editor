#ifndef ZE_EDITOR_H
#define ZE_EDITOR_H

#include "buffer.h"
#include "cursor.h"
#include "undo.h"
#include "selection.h"
#include "clipboard.h"

/**
 * @file editor.h
 * @brief main editor state and functions.
 *
 * this file defines the Editor structure, which holds the state of the text editor, including the buffer, cursor position, screen dimensions, and other relevant information. It also declares functions for initializing, running, and cleaning up the editor.
 */

/**
 * @def ZE_SEARCH_QUERY_MAX
 * @brief maximum length of a search query, excluding the null terminator.
 */
#define ZE_SEARCH_QUERY_MAX 255

/**
 * @struct Search
 * @brief holds the state of an active or last-used incremental search.
 */
typedef struct
{
    int active;                          // 1 while the search prompt is open
    char query[ZE_SEARCH_QUERY_MAX + 1]; // current query text (null terminated)
    int query_len;                       // number of characters in the query
    int saved_row;                       // cursor row to restore if search is cancelled
    int saved_col;                       // cursor col to restore if search is cancelled
    int saved_scroll;                    // scroll offset to restore if search is cancelled
} Search;

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
    History history;   // undo/redo edit history
    Search search;     // search prompt and query state
    Selection selection; // active text selection region
    Clipboard clipboard; // internal copy/cut/paste buffer
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

/**
 * @brief open the search prompt, saving the current cursor and scroll position.
 * @param ed pointer to the editor state.
 */
void editor_search_open(Editor *ed);

/**
 * @brief close the search prompt, keeping the cursor at the current match.
 * @param ed pointer to the editor state.
 */
void editor_search_close(Editor *ed);

/**
 * @brief cancel the search, restoring the cursor and scroll to where it began.
 * @param ed pointer to the editor state.
 */
void editor_search_cancel(Editor *ed);

/**
 * @brief append a character to the search query and jump to the first match.
 * @param ed pointer to the editor state.
 * @param c the character to append.
 */
void editor_search_input_char(Editor *ed, char c);

/**
 * @brief remove the last character from the search query and re-search.
 * @param ed pointer to the editor state.
 */
void editor_search_backspace(Editor *ed);

/**
 * @brief move the cursor to the next match after the current position.
 * @param ed pointer to the editor state.
 */
void editor_search_next(Editor *ed);

/**
 * @brief move the cursor to the previous match before the current position.
 * @param ed pointer to the editor state.
 */
void editor_search_prev(Editor *ed);

#endif /* ZE_EDITOR_H */
