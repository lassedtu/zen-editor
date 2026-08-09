/**
 * @file platforms/windows/fs.c
 * @brief windows-specific file system operations.
 *
 * implements the platform_fs interface using standard C file I/O. the same
 * fopen/fread/fwrite approach works on windows with the MSVC or MinGW runtime.
 */

#include "platform_fs.h"

#include <stdio.h>
#include <stdlib.h>

char *platform_fs_read_file(const char *path, int *out_len)
{
    FILE *fp = fopen(path, "rb");
    if (!fp)
        return NULL;

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *data = malloc(size);
    if (!data)
    {
        fclose(fp);
        return NULL;
    }

    int bytes_read = (int)fread(data, 1, size, fp);
    fclose(fp);

    *out_len = bytes_read;
    return data;
}

int platform_fs_write_file(const char *path, const char *data, int len)
{
    FILE *fp = fopen(path, "wb");
    if (!fp)
        return -1;

    int written = (int)fwrite(data, 1, len, fp);
    fclose(fp);

    return (written == len) ? 0 : -1;
}
