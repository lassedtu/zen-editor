#include "platform_terminal.h"
#include "keys.h"

#include <string.h>
#include <stdio.h>

/*
 * We need ginnOS syscall declarations (write, read_event, ttyctl) but
 * unistd.h defines KEY_ARROW_UP etc. as macros that conflict with ze's
 * enum Key in keys.h. Instead we declare only what we need directly.
 */

/* ginnOS syscall prototypes (from libc unistd.h) */
typedef unsigned int size_t;
typedef int ssize_t;

ssize_t write(int fd, const void *buf, size_t count);
ssize_t read(int fd, void *buf, size_t count);

#define TTY_COOKED 0
#define TTY_RAW    1
int ttyctl(int mode);

/* ginnOS key event types */
#define GINNOS_KEY_EVENT_CHAR    0
#define GINNOS_KEY_EVENT_SPECIAL 1

/* ginnOS special key codes */
#define GINNOS_KEY_ARROW_UP    1
#define GINNOS_KEY_ARROW_DOWN  2
#define GINNOS_KEY_ARROW_LEFT  3
#define GINNOS_KEY_ARROW_RIGHT 4
#define GINNOS_KEY_HOME        5
#define GINNOS_KEY_END         6
#define GINNOS_KEY_PAGE_UP     7
#define GINNOS_KEY_PAGE_DOWN   8
#define GINNOS_KEY_DELETE       10

/* escape byte (0x1B) as a named constant to avoid .base64 in GCC 16+ */
#define ESC 0x1b

/* ginnOS key event structure (matches kernel layout, 8 bytes) */
typedef struct
{
    int type;
    union
    {
        char character;
        int  special;
    };
} ginnos_key_event_t;

/**
 * read a single keyboard event from stdin (raw mode).
 * blocks until an event is available.
 */
static int ginnos_read_event(ginnos_key_event_t *event)
{
    int n = (int)read(0, (void *)event, sizeof(ginnos_key_event_t));
    if (n < (int)sizeof(ginnos_key_event_t))
        return -1;
    return 0;
}

/**
 * @file platforms/ginnos/terminal.c
 * @brief ginnOS-specific terminal operations.
 *
 * Implements the platform terminal interface using ginnOS syscalls:
 * - ttyctl() for raw/cooked mode switching
 * - read() for keyboard input (structured key events in raw mode)
 * - write() for terminal output (kernel console parses ANSI)
 *
 * ginnOS provides a VGA text-mode console (80x25) with an ANSI escape
 * sequence parser, so we emit standard ANSI sequences for cursor movement,
 * screen clearing, etc.
 */

/* --- Write buffer -------------------------------------------------------- */

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

/* --- Platform terminal interface ----------------------------------------- */

int platform_terminal_init(void)
{
    ttyctl(TTY_RAW);
    return 0;
}

void platform_terminal_cleanup(void)
{
    /* clear screen and move cursor home for a clean shell return.
     * built as a char array to avoid .base64 codegen with GCC 16+. */
    char seq[] = { ESC, '[', '2', 'J', ESC, '[', 'H' };
    write(1, seq, sizeof(seq));
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
    char seq[] = { ESC, '[', '2', 'J', ESC, '[', 'H' };
    wbuf_append(seq, (int)sizeof(seq));
}

void platform_terminal_move_cursor(int row, int col)
{
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "\033[%d;%dH", row + 1, col + 1);
    wbuf_append(buf, len);
}

void platform_terminal_write(const char *str, int len)
{
    wbuf_append(str, len);
}

int platform_terminal_read_key(void)
{
    ginnos_key_event_t ev;

    if (ginnos_read_event(&ev) != 0)
        return -1;

    if (ev.type == GINNOS_KEY_EVENT_CHAR)
    {
        unsigned char c = (unsigned char)ev.character;

        /* ginnOS sends '\n' for Enter; ze expects KEY_ENTER ('\r') */
        if (c == '\n')
            return KEY_ENTER;

        /* map backspace (ginnOS sends 0x08) to ze's KEY_BACKSPACE */
        if (c == '\b')
            return KEY_BACKSPACE;

        return (int)c;
    }

    /* GINNOS_KEY_EVENT_SPECIAL: map ginnOS special key codes to ze Key enum */
    switch (ev.special)
    {
    case GINNOS_KEY_ARROW_UP:    return KEY_ARROW_UP;
    case GINNOS_KEY_ARROW_DOWN:  return KEY_ARROW_DOWN;
    case GINNOS_KEY_ARROW_LEFT:  return KEY_ARROW_LEFT;
    case GINNOS_KEY_ARROW_RIGHT: return KEY_ARROW_RIGHT;
    case GINNOS_KEY_HOME:        return KEY_HOME;
    case GINNOS_KEY_END:         return KEY_END;
    case GINNOS_KEY_PAGE_UP:     return KEY_PAGE_UP;
    case GINNOS_KEY_PAGE_DOWN:   return KEY_PAGE_DOWN;
    case GINNOS_KEY_DELETE:       return KEY_DELETE;
    default:                     return -1;
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
    /* ginnOS VGA text mode is fixed-size, never resizes */
    return 0;
}
