#ifndef ZE_PLATFORM_TERMINAL_H
#define ZE_PLATFORM_TERMINAL_H

/**
 * @file platform_terminal.h
 * @brief platform-specific terminal operations.
 *
 * this file provides functions for initializing the terminal, reading input, and writing output in a platform-independent way.
 */

/**
 * @brief initialize the terminal for raw input and output.
 * @return 0 on success, -1 on failure (e.g., unable to set
 */
int platform_terminal_init(void);

/**
 * @brief restore the terminal to its original state.
 */
void platform_terminal_cleanup(void);

/**
 * @brief get the current size of the terminal window.
 * @param rows pointer to an integer where the number of rows will be stored.
 * @param cols pointer to an integer where the number of columns will be stored.
 * @return 0 on success, -1 on failure (e.g., unable to get terminal size).
 */
int platform_terminal_get_size(int *rows, int *cols);

/**
 * @brief clear the terminal screen and move the cursor to the top-left corner.
 */
void platform_terminal_clear(void);

/**
 * @brief move the cursor to the specified position in the terminal.
 * @param row the row index to move the cursor to (0-based).
 * @param col the column index to move the cursor to (0-based).
 */
void platform_terminal_move_cursor(int row, int col);

/**
 * @brief write a string to the terminal.
 * @param str pointer to the string to write.
 * @param len the length of the string to write.
 */
void platform_terminal_write(const char *str, int len);

/**
 * @brief read a key press from the terminal.
 * @return the key code of the pressed key, or -1 on failure (e.g, unable to read input).
 */
int platform_terminal_read_key(void);

/**
 * @brief flush any buffered output to the terminal.
 */
void platform_terminal_flush(void);

#endif /* ZE_PLATFORM_TERMINAL_H */
