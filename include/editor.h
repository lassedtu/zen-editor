#ifndef ZE_EDITOR_H
#define ZE_EDITOR_H

#include "buffer.h"
#include "cursor.h"

/*
 * Editor
 *
 * Top-level editor state and main loop.
 */

typedef struct {
    Buffer *buffer;
    Cursor cursor;
    int screen_rows;
    int screen_cols;
    int scroll_offset;
    int running;
    char *filename;
} Editor;

/* Initialize the editor */
int editor_init(Editor *ed, const char *filename);

/* Run the main editor loop */
void editor_run(Editor *ed);

/* Clean up and shut down the editor */
void editor_cleanup(Editor *ed);

#endif /* ZE_EDITOR_H */
