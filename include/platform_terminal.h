#ifndef ZE_PLATFORM_TERMINAL_H
#define ZE_PLATFORM_TERMINAL_H

/*
 * Platform Abstraction Layer — Terminal
 *
 * Provides an interface for terminal rendering and input.
 * Each platform backend must implement these functions.
 */

/* Initialize the terminal for raw mode editing */
int platform_terminal_init(void);

/* Restore the terminal to its original state */
void platform_terminal_cleanup(void);

/* Get the current terminal dimensions */
int platform_terminal_get_size(int *rows, int *cols);

/* Clear the entire screen */
void platform_terminal_clear(void);

/* Move the cursor to a specific position (0-indexed) */
void platform_terminal_move_cursor(int row, int col);

/* Write a string to the terminal at the current cursor position */
void platform_terminal_write(const char *str, int len);

/* Read a single keypress (blocking). Returns the key code. */
int platform_terminal_read_key(void);

/* Flush all buffered output to the terminal */
void platform_terminal_flush(void);

#endif /* ZE_PLATFORM_TERMINAL_H */
