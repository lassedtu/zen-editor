/**
 * @file platforms/windows/terminal.c
 * @brief Windows-specific terminal operations.
 *
 * implements the platform_terminal interface using the Win32 Console API.
 * uses SetConsoleMode for raw input, GetConsoleScreenBufferInfo for size
 * queries, and virtual terminal sequences for output rendering.
 */

#include "platform_terminal.h"
#include "keys.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static HANDLE h_stdin;      // console input handle
static HANDLE h_stdout;     // console output handle
static DWORD orig_in_mode;  // original input console mode to restore on cleanup
static DWORD orig_out_mode; // original output console mode to restore on cleanup

static int resize_flag = 0; // set when a window buffer size event is received

#define WBUF_INIT_SIZE 4096

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
        DWORD written;
        WriteConsole(h_stdout, wbuf, (DWORD)wbuf_len, &written, NULL);
        wbuf_len = 0;
    }
}

int platform_terminal_init(void)
{
    /* initialize write buffer */
    wbuf_cap = WBUF_INIT_SIZE;
    wbuf = malloc(wbuf_cap);
    wbuf_len = 0;

    h_stdin = GetStdHandle(STD_INPUT_HANDLE);
    h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);

    if (h_stdin == INVALID_HANDLE_VALUE || h_stdout == INVALID_HANDLE_VALUE)
        return -1;

    /* save original console modes */
    if (!GetConsoleMode(h_stdin, &orig_in_mode))
        return -1;
    if (!GetConsoleMode(h_stdout, &orig_out_mode))
        return -1;

    /* enable raw input: disable line input, echo, and processed input */
    DWORD in_mode = ENABLE_WINDOW_INPUT;
    if (!SetConsoleMode(h_stdin, in_mode))
        return -1;

    /* enable virtual terminal processing for ANSI escape sequences on output */
    DWORD out_mode = orig_out_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(h_stdout, out_mode))
    {
        /* if VT processing is not supported, fall back to original mode */
        SetConsoleMode(h_stdout, orig_out_mode);
    }

    /* enter alternate screen buffer so the main screen is preserved on exit */
    DWORD written;
    WriteConsole(h_stdout, "\x1b[?1049h", 8, &written, NULL);

    return 0;
}

void platform_terminal_cleanup(void)
{
    /* leave alternate screen buffer to restore the original terminal content */
    DWORD written;
    WriteConsole(h_stdout, "\x1b[?1049l", 8, &written, NULL);

    SetConsoleMode(h_stdin, orig_in_mode);
    SetConsoleMode(h_stdout, orig_out_mode);
    free(wbuf);
    wbuf = NULL;
    wbuf_len = 0;
    wbuf_cap = 0;
}

int platform_terminal_has_resized(void)
{
    if (resize_flag)
    {
        resize_flag = 0;
        return 1;
    }
    return 0;
}

int platform_terminal_get_size(int *rows, int *cols)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(h_stdout, &csbi))
    {
        *rows = 24;
        *cols = 80;
        return -1;
    }
    *cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    *rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
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
    INPUT_RECORD record;
    DWORD events_read;

    for (;;)
    {
        if (!ReadConsoleInput(h_stdin, &record, 1, &events_read))
            return -1;

        /* handle window resize events */
        if (record.EventType == WINDOW_BUFFER_SIZE_EVENT)
        {
            resize_flag = 1;
            continue;
        }

        if (record.EventType != KEY_EVENT)
            continue;

        if (!record.Event.KeyEvent.bKeyDown)
            continue;

        WORD vk = record.Event.KeyEvent.wVirtualKeyCode;
        char ch = record.Event.KeyEvent.uChar.AsciiChar;

        /* handle special keys via virtual key codes */
        switch (vk)
        {
        case VK_UP:
            return KEY_ARROW_UP;
        case VK_DOWN:
            return KEY_ARROW_DOWN;
        case VK_LEFT:
            return KEY_ARROW_LEFT;
        case VK_RIGHT:
            return KEY_ARROW_RIGHT;
        case VK_HOME:
            return KEY_HOME;
        case VK_END:
            return KEY_END;
        case VK_PRIOR:
            return KEY_PAGE_UP;
        case VK_NEXT:
            return KEY_PAGE_DOWN;
        case VK_DELETE:
            return KEY_DELETE;
        case VK_RETURN:
            return KEY_ENTER;
        case VK_ESCAPE:
            return KEY_ESCAPE;
        case VK_BACK:
            return KEY_BACKSPACE;
        }

        /* handle ctrl key combinations */
        if (record.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
        {
            if (ch >= 1 && ch <= 26)
                return ch;
        }

        /* regular character */
        if (ch != 0)
            return (int)ch;

        /* ignore unrecognized keys (shift, alt alone, etc.) */
    }
}