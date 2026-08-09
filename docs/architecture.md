# Architecture

This document describes the internal architecture of ze. The editor is split into two layers: a platform-independent core that contains all text-editing logic, and a platform layer that provides OS-specific terminal and filesystem access.

## Layer Overview

```
┌────────────────────────────────────────────────┐
│                  Editor Core                   │
│  (buffer, cursor, renderer, editor loop)       │
├────────────────────────────────────────────────┤
│              Command Layer                     │
│  (keys.h, keymap, command)                     │
├────────────────────────────────────────────────┤
│             Platform Interface                 │
│  (platform_terminal.h, platform_fs.h)          │
├────────────────────────────────────────────────┤
│          Platform Implementation               │
│  (unix/, ginnos/, windows/ ...)                │
└────────────────────────────────────────────────┘
```

The core never includes OS-specific headers. It only calls functions declared in the platform interface headers. This means the same core compiles unchanged on any target — you swap only the platform backend.

The command layer sits between raw input and state mutation. It translates key codes into abstract commands, then executes those commands against the editor state. This decoupling enables future undo/redo, configurable bindings, and macro replay.

## Core Modules

All core source lives in `src/`, with public headers in `include/`.

### buffer (buffer.h / buffer.c)

The text buffer. Stores file contents as a dynamic array of `Line` structs, each holding a `char *` and a length. Provides insert/delete at character and line level, plus file load/save (delegated to the platform filesystem layer).

### cursor (cursor.h / cursor.c)

Cursor position and movement logic. Operates on a `Cursor` struct (row, col) relative to the buffer. Handles directional movement, home/end, line wrapping, and clamping to valid bounds.

### renderer (renderer.h / renderer.c)

Draws the buffer contents and status bar to the terminal. Manages scrolling by accepting a scroll offset. Uses only `platform_terminal_*` calls for output — no direct terminal escape codes beyond what the platform layer provides.

### editor (editor.h / editor.c)

Top-level state and main loop. The `Editor` struct ties together the buffer, cursor, screen dimensions, scroll offset, and filename. `editor_run()` is the main loop: scroll → draw → draw status → reposition cursor → flush → read key → execute command.

Input is read via `platform_terminal_read_key()`, translated through `keymap_translate()`, and executed by `editor_execute()`. The editor itself no longer contains key dispatch logic.

## Command Layer

The command layer decouples "what key was pressed" from "what action to perform." It consists of three files:

### keys (keys.h)

Centralized key code definitions. Both the platform terminal layer (which produces key codes) and the keymap layer (which consumes them) include this header as the single source of truth. Defines:

- `KEY_CTRL(k)` macro for control key combinations.
- `enum Key` with named constants for all special keys (arrows, home, end, page up/down, delete, backspace, enter, escape).

### keymap (keymap.h / keymap.c)

Translates raw key codes into abstract `Command` structs. The current mapping is hard-coded but isolated here so that future configurable bindings only need to change this file.

### command (command.h / command.c)

Defines `CommandType` (an enum of all possible editor actions) and the `Command` struct (type + optional character payload). Implements `editor_execute()`, which performs the corresponding state mutation for each command type.

This is the future hook point for:
- **Undo/redo:** record each executed command.
- **Macros:** replay a sequence of commands.
- **Configurable bindings:** swap the keymap without touching execution logic.

### Input-to-action pipeline

```
platform_terminal_read_key()   →   keymap_translate()   →   editor_execute()
       (raw key code)                  (Command)             (state mutation)
```

## Platform Interface

The platform interface is defined by two headers in `include/`:

### platform_terminal.h

Terminal I/O abstraction. Functions:

| Function | Purpose |
|----------|---------|
| `platform_terminal_init` | Enter raw mode |
| `platform_terminal_cleanup` | Restore original terminal state |
| `platform_terminal_get_size` | Query terminal dimensions |
| `platform_terminal_clear` | Clear screen |
| `platform_terminal_move_cursor` | Position cursor (0-based row, col) |
| `platform_terminal_write` | Buffer output bytes |
| `platform_terminal_read_key` | Block and return next key code |
| `platform_terminal_flush` | Flush buffered output to terminal |

### platform_fs.h

Filesystem abstraction. Functions:

| Function | Purpose |
|----------|---------|
| `platform_fs_read_file` | Read entire file into a malloc'd buffer |
| `platform_fs_write_file` | Write buffer to file, overwriting |

## Platform Implementations

Each platform lives in `platforms/<name>/` and implements both interfaces.

### Unix (platforms/unix/)

- `terminal.c` — Uses `termios` for raw mode, `ioctl` for terminal size, `read()`/`write()` for I/O. Implements a write buffer that batches output and flushes with a single `write()` call.
- `fs.c` — Standard `fopen`/`fread`/`fwrite` file access.

### ginnOS (platforms/ginnos/) — future

Will implement the same interfaces using ginnOS kernel syscalls. No POSIX dependency.

### Windows (platforms/windows/) — future

Will implement the interfaces using the Win32 console API.

## Adding a New Platform

1. Create a directory under `platforms/` (e.g., `platforms/myos/`).
2. Implement all functions declared in `platform_terminal.h` and `platform_fs.h`.
3. Build with `make PLATFORM=myos`. The Makefile automatically discovers `.c` files under `platforms/<PLATFORM>/`.

No changes to core source or headers required.

## Data Flow

A simplified view of one iteration of the editor loop:

```
editor_run()
  ├── editor_scroll()              adjust scroll_offset so cursor is visible
  ├── renderer_draw()              write buffer lines to terminal (buffered)
  ├── renderer_draw_status()       write status bar (buffered)
  ├── platform_terminal_move_cursor()  final cursor position (buffered)
  ├── platform_terminal_flush()    write all buffered output at once
  └── editor_process_key()
        ├── platform_terminal_read_key()  read raw key code
        ├── keymap_translate()            map to Command
        └── editor_execute()              mutate editor state
```

## Build System

The Makefile uses automatic source discovery:

- All `.c` files in `src/` are compiled (core).
- All `.c` files in `platforms/$(PLATFORM)/` are compiled (backend).
- Object files mirror the source tree under `build/`.
- Header dependencies are tracked via `-MMD -MP` for incremental rebuilds.

Set the platform with `make PLATFORM=<name>`. Default is `unix`.
