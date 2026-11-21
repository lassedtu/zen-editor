#include "platform_fs.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/**
 * @file platforms/ginnos/fs.c
 * @brief ginnOS-specific file system operations.
 *
 * implements file read/write using ginnOS syscalls:
 * - open(), close(), read(), write()
 * - lseek() for determining file size
 * - O_CREAT | O_TRUNC to create or overwrite a file
 */

char *platform_fs_read_file(const char *path, int *out_len)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return NULL;

    /* determine file size using lseek (no stat syscall) */
    int size = lseek(fd, 0, SEEK_END);
    if (size < 0)
    {
        close(fd);
        return NULL;
    }
    lseek(fd, 0, SEEK_SET);

    char *data = malloc((size_t)size + 1);
    if (!data)
    {
        close(fd);
        return NULL;
    }

    int total = 0;
    while (total < size)
    {
        int n = read(fd, data + total, size - total);
        if (n <= 0)
            break;
        total += n;
    }

    close(fd);
    *out_len = total;
    return data;
}

int platform_fs_write_file(const char *path, const char *data, int len)
{
    /* create the file, or empty it if it exists */
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        return -1;

    int total = 0;
    while (total < len)
    {
        int n = write(fd, data + total, len - total);
        if (n <= 0)
        {
            close(fd);
            return -1;
        }
        total += n;
    }

    close(fd);
    return 0;
}
