#ifndef ZE_BUFFER_H
#define ZE_BUFFER_H

/*
 * Text Buffer
 *
 * Stores the contents of a file as an array of lines.
 * All text manipulation goes through this module.
 */

typedef struct {
    char *chars;
    int len;
} Line;

typedef struct {
    Line *lines;
    int num_lines;
    int modified;
} Buffer;

/* Create a new empty buffer */
Buffer *buffer_create(void);

/* Free all memory associated with a buffer */
void buffer_free(Buffer *buf);

/* Insert a character at (row, col) */
void buffer_insert_char(Buffer *buf, int row, int col, char c);

/* Delete the character at (row, col) */
void buffer_delete_char(Buffer *buf, int row, int col);

/* Insert a new line by splitting the line at (row, col) */
void buffer_insert_newline(Buffer *buf, int row, int col);

/* Delete a line and merge it with the previous line */
void buffer_delete_line(Buffer *buf, int row);

/* Load file contents into the buffer */
int buffer_load(Buffer *buf, const char *filename);

/* Save buffer contents to a file */
int buffer_save(Buffer *buf, const char *filename);

#endif /* ZE_BUFFER_H */
