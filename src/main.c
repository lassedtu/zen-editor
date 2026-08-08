#include "editor.h"
#include <stdio.h>

/**
 * main entry point for the zen editor
 * @param argc argument count
 * @param argv argument vector
 * @return exit status code
 */
int main(int argc, char *argv[])
{
    Editor ed;
    const char *filename = NULL;

    if (argc >= 2)
    {
        filename = argv[1];
    }

    if (editor_init(&ed, filename) != 0)
    {
        fprintf(stderr, "ze: failed to initialize editor\n");
        return 1;
    }

    editor_run(&ed);
    editor_cleanup(&ed);

    return 0;
}
