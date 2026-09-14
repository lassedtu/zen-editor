/**
 * @file test_main.c
 * @brief test runner for ze.
 *
 * includes all test files and executes registered tests.
 */

#include "ze_test.h"

/* test files are included here to allow auto-registration */
#include "test_buffer.c"
#include "test_cursor.c"
#include "test_undo.c"
#include "test_search.c"
#include "test_selection.c"

int main(void)
{
    RUN_TESTS();
}
