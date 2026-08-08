#ifndef ZE_PLATFORM_FS_H
#define ZE_PLATFORM_FS_H

/*
 * Platform Abstraction Layer — Filesystem
 *
 * Provides an interface for file operations.
 * Each platform backend must implement these functions.
 */

/* Read an entire file into a buffer. Caller must free the returned pointer.
 * Sets *out_len to the number of bytes read. Returns NULL on failure. */
char *platform_fs_read_file(const char *path, int *out_len);

/* Write a buffer to a file. Returns 0 on success, -1 on failure. */
int platform_fs_write_file(const char *path, const char *data, int len);

#endif /* ZE_PLATFORM_FS_H */
