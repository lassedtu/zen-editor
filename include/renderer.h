#ifndef ZE_RENDERER_H
#define ZE_RENDERER_H

#include "buffer.h"
#include "cursor.h"
#include "selection.h"
#include "config.h"

/**
 * @file renderer.h
 * @brief functions for rendering the buffer and status bar to the terminal.
 *
 * this file contains the declarations for functions that handle drawing the contents of the buffer to the terminal, as well as rendering the status bar at the bottom of the screen. The renderer is responsible for managing scrolling, cursor positioning, and ensuring that the terminal display reflects the current state of the editor.
 */

/**
 * @brief draw the buffer contents to the terminal, handling scrolling and cursor positioning
 * @param buf pointer to the buffer to draw
 * @param cur pointer to the cursor position
 * @param screen_rows number of rows in the terminal
 * @param screen_cols number of columns in the terminal
 * @param scroll_offset the number of lines to scroll from the top of the buffer
 * @param highlight optional query string whose matches are highlighted; NULL or empty disables highlighting
 * @param sel optional active selection whose region is drawn with inverted video; NULL or inactive disables selection drawing
 * @param cfg pointer to the settings of the user, for the tab size and the colors
 */
void renderer_draw(Buffer *buf, Cursor *cur, int screen_rows, int screen_cols,
                   int scroll_offset, const char *highlight,
                   const Selection *sel, const Config *cfg);

/**
 * @brief draw the status bar at the bottom of the screen, showing filename, line count, and cursor position
 * @param filename the name of the file being edited
 * @param num_lines the total number of lines in the buffer
 * @param cur_row the current row of the cursor
 * @param screen_rows the total number of rows in the terminal
 * @param cfg pointer to the settings of the user, for the status bar color
 */
void renderer_draw_status(const char *filename, int num_lines, int cur_row,
                          int screen_rows, const Config *cfg);

/**
 * @brief find the visual column of a character column on a line.
 *
 * the function counts the columns from the start of the line to the character
 * column. it counts one column for a normal character. it counts the columns to
 * the next tab stop for a tab. the editor uses this value to put the cursor at
 * the correct place when the line has tabs.
 *
 * @param line pointer to the line to read.
 * @param col the character column of the cursor.
 * @param tab_size number of columns for one tab stop.
 * @return the visual column for the character column.
 */
int renderer_visual_col(const Line *line, int col, int tab_size);

/**
 * @brief draw the search prompt on the status row, replacing the status bar.
 * @param query the current search query text.
 * @param match_count the number of matches for the query in the buffer.
 * @param screen_rows the total number of rows in the terminal.
 * @param screen_cols the total number of columns in the terminal.
 */
void renderer_draw_search_prompt(const char *query, int match_count,
                                 int screen_rows, int screen_cols);

#endif /* ZE_RENDERER_H */
