# Contributing

Guide for contributing to ze. Covers build setup, coding style, documentation rules, and workflow.

## Building

```sh
make                    # Build for current platform (defaults to unix)
make PLATFORM=unix      # Explicitly target specific platform
make run                # Build and run
make clean              # Remove build artifacts
```

The binary is output to `build/ze`. The Makefile handles incremental rebuilds via tracked header dependencies.

## Project Layout

```
src/
├── main.c          Entry point
├── editor.c        Editor state and main loop
├── buffer.c        Text buffer (line storage, insert/delete)
├── cursor.c        Cursor movement and bounds checking
├── renderer.c      Screen drawing and scrolling
├── command.c       Command execution (state mutation)
└── keymap.c        Key-to-command translation

include/
├── editor.h        Editor struct and lifecycle functions
├── buffer.h        Buffer and Line structs, buffer operations
├── cursor.h        Cursor struct and movement functions
├── renderer.h      Rendering functions
├── command.h       CommandType enum, Command struct, editor_execute()
├── keymap.h        keymap_translate()
├── keys.h          Centralized key code definitions
├── platform_terminal.h   Terminal I/O interface
└── platform_fs.h         Filesystem interface

platforms/          Platform-specific implementations (one directory per target)
tests/              Tests
docs/               Documentation
```

See [architecture.md](architecture.md) for how these layers interact.

## Coding Style

### Formatting

- **Brace style:** Allman (opening brace on its own line)
    - Example:
        ```c
        void function(void)
        {
            ...
        }
        ```
- **Indentation:** 4 spaces, no tabs.
    - Example:
        ```c
        void foo(void)
        {
            bar();
        }
        ```
- **Line length:** Keep reasonable (~100 chars), no hard limit.
- **Blank lines:** One blank line between function definitions. No trailing whitespace.
    - Example:
        ```c
        void foo(void);

        void bar(void);
        ```

### Naming

- **Functions and variables:** `lowercase_snake_case`.
- **Structs and enums:** `PascalCase` for type names (e.g., `Buffer`, `Cursor`, `EditorKey`).
- **Macros:** `UPPER_SNAKE_CASE`.
- **Include guards:** `ZE_<FILENAME>_H` (e.g., `ZE_BUFFER_H`).
- **Platform functions:** Prefixed with `platform_terminal_`, `platform_fs_`, etc (depending on their header file).

### Includes

Order includes as follows, separated by blank lines:

1. The corresponding header (e.g., `buffer.c` includes `"buffer.h"` first).
2. Other project headers (e.g., `"platform_fs.h"`).
3. System/standard library headers (e.g., `<stdlib.h>`, `<string.h>`).

Example:
``` c
#include "header.h"

#include "other.h"
#include "another.h"

#include <stdlib.h>
#include <string.h>
```

### General

- Keep functions short and focused. If a function grows beyond ~50 lines, consider splitting it (no hard limit).
- Prefer early returns for error handling over deep nesting.
- Use `static` for all functions and variables that are not part of the public interface.

## Documentation Rules

Every piece of code must be documented following these rules. This is not optional.

### File-level documentation

Every `.c` and `.h` file must start with a file-level comment block:

```c
/**
 * @file filename.c
 * @brief one-line description of what this file contains.
 *
 * optional longer explanation if needed.
 */
```

### Struct and enum documentation

Every struct and enum must be documented:

```c
/**
 * @struct StructName
 * @brief one-line description.
 */
typedef struct
{
    int field; // inline description of this field
} StructName;

/**
 * @enum EnumName
 * @brief one-line description.
 */
enum EditorKey
{
    ITEM; // inline description of this item
};
```

### Function documentation

Every function must be documented with `@brief`, all `@param` entries, and `@return` if it returns a value:

```c
/**
 * @brief one-line description of what the function does.
 * @param name description of this parameter.
 * @param other description of this parameter.
 * @return what the function returns and when (e.g., "0 on success, -1 on failure").
 */
int my_function(int name, int other);
```

### Where to place documentation

- **Header files:** Document all public functions, structs, and enums in the header where they are declared.
- **Source files:** Document `static`/private functions directly above their definition in the `.c` file. Also document functions in `.c` files that have no corresponding header (e.g., `main.c`).

Do not duplicate documentation in both the header and the source file. The header is the "official" location for public API docs.

### Style within doc comments

- Keep `@brief` to one line. Use the long description area below it for additional context.
- Describe parameters in terms of what they represent, not their C type.

## Workflow

### Branches

- `main` is the stable branch. Don't push directly.
- Create a branch for each feature or fix (e.g., `feature/undo`, `fix/cursor-wrap`).
- Keep commits focused: one logical change per commit.
- Create pull requests and do not merge before code has been reviewed.

### Commit Messages

Format: `<area>: <short description>`

Examples:
```
buffer: fix off-by-one in delete_line

platform/unix: handle SIGWINCH for terminal resize

docs: add contributing guide
```

Keep the first line under 72 characters. Add a blank line and a longer explanation below if the change needs context.

### Before Pushing

1. `make clean && make` — ensure a clean build with no warnings.
2. Run tests if they exist for the area you changed.
3. Read through your diff. Remove debug prints, commented-out code, and unrelated changes.
