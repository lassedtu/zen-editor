/**
 * @file test_buffer.c
 * @brief unit tests for the buffer module.
 */

#include "buffer.h"
#include <stdlib.h>

TEST(buffer_create_has_one_empty_line)
{
    Buffer *buf = buffer_create();
    ASSERT(buf != NULL);
    ASSERT_EQ(buf->num_lines, 1);
    ASSERT_EQ(buf->lines[0].len, 0);
    ASSERT_EQ(buf->modified, 0);
    buffer_free(buf);
}
