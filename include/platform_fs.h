#ifndef ZE_PLATFORM_FS_H
#define ZE_PLATFORM_FS_H

/**
 * @file platform_fs.h
 * @brief Platform-specific file system operations.
 *
 * This file provides functions for reading from and writing to files in a platform-independent way.
 */

/**
 * @brief read the contents of a file into a dynamically allocated buffer.
 * @param path the path to the file to read.
 * @param out_len pointer to an integer where the length of the read data will be stored
 * @return pointer to the buffer containing the file contents, or NULL on failure. the caller is responsible for freeing the returned buffer.
 */
char *platform_fs_read_file(const char *path, int *out_len);

/**
 * @brief write data to a file, overwriting it if it already exists.
 * @param path the path to the file to write.
 * @param data pointer to the data to write.
 * @param len the length of the data to write.
 * @return 0 on success, -1 on failure (e.g., unable to open file for writing).
 */
int platform_fs_write_file(const char *path, const char *data, int len);

#endif /* ZE_PLATFORM_FS_H */
