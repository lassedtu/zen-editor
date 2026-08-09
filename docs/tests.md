# Testing

Guide for writing and running tests in ze.

## Running Tests

```sh
make test               # Build and run all tests
```

The test binary is output to `build/ze_tests`. A non-zero exit code indicates failure.

## Framework

Tests use a custom single-header framework at `tests/ze_test.h`. No external dependencies.

### Defining a Test

```c
#include "ze_test.h"

TEST(my_feature_works)
{
    int result = 1 + 1;
    ASSERT_EQ(result, 2);
}
```

Tests auto-register. Just define them with `TEST(name)` and they will be picked up by the runner.

### Available Macros

| Macro | Purpose |
|-------|---------|
| `TEST(name)` | Define and register a test case |
| `ASSERT(expr)` | Fail if `expr` is false |
| `ASSERT_EQ(a, b)` | Fail if `a != b` (integer comparison, prints both values) |
| `ASSERT_STR_EQ(a, b)` | Fail if strings differ (prints both values) |
| `RUN_TESTS()` | Execute all registered tests, print summary, return exit code |

On failure, each macro prints the file, line number, and the failing expression or values.

### Test Runner

`tests/test_main.c` is the entry point. It includes all test files and calls `RUN_TESTS()`:

```c
#include "ze_test.h"
#include "test_buffer.c"
#include "test_cursor.c"

int main(void)
{
    RUN_TESTS();
}
```

To add a new test file, create `tests/test_<module>.c` and add an include in `test_main.c`.

## File Layout

```
tests/
├── ze_test.h          Framework header
├── test_main.c        Runner (includes test files, calls RUN_TESTS)
├── test_buffer.c      Buffer unit tests
└── test_cursor.c      Cursor unit tests
```

## Writing Good Tests

- Test one behavior per `TEST()`. Keep them short and focused on one thing.
- Name tests descriptively: `buffer_insert_char_at_start`, not `test1`.
- Each test should create its own state and clean up after itself (free buffers, etc.).
- Test edge cases: empty buffers, position 0, end of line, single-character lines.

## Documenting Tests

If a test's name alone doesn't fully explain what's being verified or why, add a short comment block above the test body:

```c
TEST(buffer_delete_char_at_end_of_line)
{
    /* deleting at col == len should be a no-op since there is no
       character at that position to remove */
    Buffer *buf = buffer_create();
    buffer_insert_char(buf, 0, 0, 'a');
    buffer_delete_char(buf, 0, 1);
    ASSERT_EQ(buf->lines[0].len, 1);
    buffer_free(buf);
}
```

Guidelines:
- Keep comments brief, one or two lines explaining the scenario or why it matters.
- Skip the comment if the test name is self-explanatory (e.g., `buffer_create_has_one_empty_line`).
- Focus on intent (what behavior is being tested, not how the code works internally).

## What to Test

The following modules have pure logic suitable for unit testing:

- **buffer** — insert/delete char, newline split/merge, load/save round-trip, capacity growth.
- **cursor** — movement, bounds clamping, line wrapping.
- **undo** (future) — push/pop operations, undo/redo correctness.

Renderer and terminal code are I/O-heavy and tested manually.
