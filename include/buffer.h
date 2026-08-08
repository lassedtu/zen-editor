#ifndef ZE_BUFFER_H
#define ZE_BUFFER_H

/**
 * @file buffer.h
 * @brief buffer data structure for text editing.
 *
 * this file defines the Buffer and Line structures, along with functions to manipulate them.
 */

/**
 * @struct Line
 * @brief represents a single line of text in the buffer.
 */
typedef struct
{
    char *chars; // pointer to the character array for this line
    int len;     // length of the line (number of characters)
} Line;

/**
 * @struct Buffer
 * @brief represents the entire text buffer, consisting of multiple lines.
 */
typedef struct
{
    Line *lines;   // pointer to an array of Line structures
    int num_lines; // number of lines in the buffer
    int modified;  // flag indicating if the buffer has been modified since last save
} Buffer;

/**
 * @brief Create a new empty buffer.
 * @return pointer to the newly created Buffer, or NULL on failure.
 */
Buffer *buffer_create(void);

/**
 * @brief free all memory associated with a buffer.
 * @param buf pointer to the Buffer to be freed.
 */
void buffer_free(Buffer *buf);

/**
 * @brief insert a character at the specified position in the buffer.
 * @param buf pointer to the Buffer.
 * @param row the row index where the character should be inserted.
 * @param col the column index where the character should be inserted.
 * @param c the character to insert.
 */
void buffer_insert_char(Buffer *buf, int row, int col, char c);

/**
 * @brief delete a character at the specified position in the buffer.
 * @param buf pointer to the Buffer.
 * @param row the row index of the character to delete.
 * @param col the column index of the character to delete.
 */
void buffer_delete_char(Buffer *buf, int row, int col);

/**
 * @brief insert a newline at the specified position in the buffer, splitting the line.
 * @param buf pointer to the Buffer.
 * @param row the row index where the newline should be inserted.
 * @param col the column index where the newline should be inserted.
 */
void buffer_insert_newline(Buffer *buf, int row, int col);

/**
 * @brief delete a line from the buffer, merging it with the previous line if applicable.
 * @param buf pointer to the Buffer.
 * @param row the row index of the line to delete.
 */
void buffer_delete_line(Buffer *buf, int row);

/**
 * @brief load the contents of a file into the buffer, replacing any existing content.
 * @param buf pointer to the Buffer.
 * @param filename the name of the file to load.
 * @return 0 on success, -1 on failure (e.g., file not found
 */
int buffer_load(Buffer *buf, const char *filename);

/**
 * @brief save the contents of the buffer to a file.
 * @param buf pointer to the Buffer.
 * @param filename the name of the file to save to.
 * @return 0 on success, -1 on failure (e.g., unable to write
 */
int buffer_save(Buffer *buf, const char *filename);

#endif /* ZE_BUFFER_H */
