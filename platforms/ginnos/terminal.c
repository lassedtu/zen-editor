#include "platform_terminal.h"
#include "keys.h"

#include <unistd.h>
#include <string.h>
#include <stdio.h>

/**
 * @file platforms/ginnos/terminal.c
 * @brief ginnOS-specific terminal operations.
 *
 * implements the platform terminal interface using ginnOS syscalls:
 * - ttyctl() for raw/cooked mode switching
 * - read_event() for keyboard input (structured key events)
 * - write() for terminal output (kernel console parses ANSI)
 *
 * ginnOS provides a VGA text-mode console (80x25) with an ANSI escape
 * sequence parser, so we emit standard ANSI sequences for cursor movement,
 * screen clearing, etc.
 */

/* ─── Write buffer ────────────────────────────────────────────────────────── */

#define WBUF_SIZE 4096

static char wbuf[WBUF_SIZE];
static int wbuf_len = 0;

/**
 * @brief append data to the write buffer, flushing if necessary.
 * @param s pointer to data to append.
 * @param len number of bytes to append.
 */
static void wbuf_append(const char *s, int len)
{
    /* flush if it won't fit */
    if (wbuf_len + len > WBUF_SIZE)
        platform_terminal_flush();

    /* if still too large for the buffer, write directly */
    if (len > WBUF_SIZE)
    {
        write(1, s, len);
        return;
    }

    memcpy(&wbuf[wbuf_len], s, len);
    wbuf_len += len;
}

int platform_terminal_init(void)
{
    ttyctl(TTY_RAW);
    return 0;
}

void platform_terminal_cleanup(void)
{
    platform_terminal_flush();
    ttyctl(TTY_COOKED);
}

int platform_terminal_get_size(int *rows, int *cols)
{
    /* ginnOS VGA text mode is fixed at 80x25 */
    *rows = 25;
    *cols = 80;
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
    key_event_t ev;

    if (read_event(&ev) != 0)
        return -1;

    if (ev.type == KEY_EVENT_CHAR)
    {
        unsigned char c = (unsigned char)ev.character;

        /* map backspace (ginnOS sends 0x08) to ze's KEY_BACKSPACE */
        if (c == '\b' || c == 127)
            return KEY_BACKSPACE;

        return (int)c;
    }

    /* KEY_EVENT_SPECIAL: map ginnOS special key codes to ze Key enum */
    switch (ev.special)
    {
    case 1:
        return KEY_ARROW_UP; /* ginnOS KEY_ARROW_UP */
    case 2:
        return KEY_ARROW_DOWN; /* ginnOS KEY_ARROW_DOWN */
    case 3:
        return KEY_ARROW_LEFT; /* ginnOS KEY_ARROW_LEFT */
    case 4:
        return KEY_ARROW_RIGHT; /* ginnOS KEY_ARROW_RIGHT */
    case 5:
        return KEY_HOME; /* ginnOS KEY_HOME */
    case 6:
        return KEY_END; /* ginnOS KEY_END */
    case 7:
        return KEY_PAGE_UP; /* ginnOS KEY_PAGE_UP */
    case 8:
        return KEY_PAGE_DOWN; /* ginnOS KEY_PAGE_DOWN */
    case 10:
        return KEY_DELETE; /* ginnOS KEY_DELETE */
    default:
        return -1;
    }
}

void platform_terminal_flush(void)
{
    if (wbuf_len > 0)
    {
        write(1, wbuf, wbuf_len);
        wbuf_len = 0;
    }
}

int platform_terminal_has_resized(void)
{
    /* ginnOS VGA text mode is fixed-size never resizes (FOR NOW!) */
    return 0;
}
