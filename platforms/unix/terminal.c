#include "platform_terminal.h"
#include "keys.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

/**
 * @file platforms/unix/terminal.c
 * @brief Unix-specific terminal operations.
 *
 * this file implements the platform_terminal_init, platform_terminal_cleanup, platform_terminal_get_size, platform_terminal_clear, platform_terminal_move_cursor, platform_terminal_write, platform_terminal_read_key, and platform_terminal_flush functions for Unix-like systems.
 */

static struct termios orig_termios; // original terminal settings to restore on cleanup

#define WBUF_INIT_SIZE 4096 // write buffer for terminal output

static char *wbuf = NULL; // write buffer for terminal output
static int wbuf_len = 0;  // current length of data in the write buffer
static int wbuf_cap = 0;  // current capacity of the write buffer

/**
 * @brief append data to the write buffer, reallocating if necessary.
 * @param s pointer to the data to append.
 * @param len the length of the data to append.
 */
static void wbuf_append(const char *s, int len)
{
    if (wbuf_len + len > wbuf_cap)
    {
        wbuf_cap = (wbuf_len + len) * 2;
        wbuf = realloc(wbuf, wbuf_cap);
    }
    memcpy(&wbuf[wbuf_len], s, len);
    wbuf_len += len;
}

void platform_terminal_flush(void)
{
    if (wbuf_len > 0)
    {
        write(STDOUT_FILENO, wbuf, wbuf_len);
        wbuf_len = 0;
    }
}

int platform_terminal_init(void)
{
    /* initialize write buffer */
    wbuf_cap = WBUF_INIT_SIZE;
    wbuf = malloc(wbuf_cap);
    wbuf_len = 0;

    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
        return -1;

    struct termios raw = orig_termios;

    /* input flags: disable break signal, CR to NL, parity, strip, flow ctrl */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

    /* output flags: disable post-processing */
    raw.c_oflag &= ~(OPOST);

    /* control flags: set 8-bit chars */
    raw.c_cflag |= (CS8);

    /* local flags: disable echo, canonical mode, signals, extended input */
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

    /* read returns after 1 byte, with a timeout of 100ms */
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        return -1;

    return 0;
}

void platform_terminal_cleanup(void)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    free(wbuf);
    wbuf = NULL;
    wbuf_len = 0;
    wbuf_cap = 0;
}

int platform_terminal_get_size(int *rows, int *cols)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
    {
        *rows = 24;
        *cols = 80;
        return -1;
    }
    *rows = ws.ws_row;
    *cols = ws.ws_col;
    return 0;
}

void platform_terminal_clear(void)
{
    wbuf_append("\x1b[2J", 4);
    wbuf_append("\x1b[H", 3);
}

void platform_terminal_move_cursor(int row, int col)
{
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "\x1b[%d;%dH", row + 1, col + 1);
    wbuf_append(buf, len);
}

void platform_terminal_write(const char *str, int len)
{
    wbuf_append(str, len);
}

int platform_terminal_read_key(void)
{
    int nread;
    char c;

    while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
    {
        if (nread == -1)
            return -1;
    }

    /* handle escape sequences */
    if (c == '\x1b')
    {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1)
            return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1)
            return '\x1b';

        if (seq[0] == '[')
        {
            if (seq[1] >= '0' && seq[1] <= '9')
            {
                if (read(STDIN_FILENO, &seq[2], 1) != 1)
                    return '\x1b';
                if (seq[2] == '~')
                {
                    switch (seq[1])
                    {
                    case '1':
                        return KEY_HOME;
                    case '3':
                        return KEY_DELETE;
                    case '4':
                        return KEY_END;
                    case '5':
                        return KEY_PAGE_UP;
                    case '6':
                        return KEY_PAGE_DOWN;
                    case '7':
                        return KEY_HOME;
                    case '8':
                        return KEY_END;
                    }
                }
            }
            else
            {
                switch (seq[1])
                {
                case 'A':
                    return KEY_ARROW_UP;
                case 'B':
                    return KEY_ARROW_DOWN;
                case 'C':
                    return KEY_ARROW_RIGHT;
                case 'D':
                    return KEY_ARROW_LEFT;
                case 'H':
                    return KEY_HOME;
                case 'F':
                    return KEY_END;
                }
            }
        }
        return '\x1b';
    }

    return (int)c;
}
