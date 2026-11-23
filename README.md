# ze (Zen Editor)
> *Being developed with the help of [mashdtu](https://github.com/mashdtu).*

A terminal-based text editor written in C, designed to be portable across operating systems.

The editor is developed on macOS/Linux while being architected from the start to run as a native userspace application on [ginnOS](https://github.com/lassedtu/ginnOS).

## Architecture

The editor core contains all text-editing logic and has no knowledge of the underlying operating system. The platform layer provides terminal rendering, keyboard input, file access, and system interaction, which allows the same codebase to target multiple platforms by swapping only the backend.

## Building

```sh
make                    # Build for current platform (defaults to unix)
make PLATFORM=unix      # Explicitly build for specific platform
make run                # Build and run
make clean              # Remove build artifacts
```

The binary is output to `build/ze`.

## Usage

```sh
./build/ze              # Open with an empty buffer
./build/ze file.txt     # Open a file
```

### Key Bindings

| Key        | Action                  |
|------------|-------------------------|
| Arrow keys | Move cursor             |
| Alt+Left / Alt+Right | Move by word  |
| Home / End | Jump to line start/end  |
| Enter      | Insert new line         |
| Backspace  | Delete character before |
| Delete     | Delete character at     |
| Shift+Arrow | Select text            |
| Alt+Shift+Left / Alt+Shift+Right | Select by word |
| Shift+Home / Shift+End | Select to line start/end |
| Ctrl+A     | Select all              |
| Ctrl+C     | Copy selection          |
| Ctrl+X     | Cut selection           |
| Ctrl+V     | Paste                   |
| Ctrl+Z     | Undo                    |
| Ctrl+Y     | Redo                    |
| Ctrl+F     | Search                  |
| Ctrl+S     | Save file               |
| Ctrl+Q     | Quit                    |

While searching: type to filter, Enter keeps the match, Esc cancels, and the
arrow keys (or Ctrl+N / Ctrl+P) jump between matches.

### macOS Command Key

The Command key does not reach a terminal program. The terminal emulator
handles it and does not send a byte for it. So the editor cannot bind
Command+C, Command+V, Command+Z, or Command+Y directly. Use the Control keys
above, or map the Command shortcuts in your terminal emulator so that each
Command combination sends the matching Control byte (for example, map
Command+C to send Control+C). Command+Q closes the terminal window and cannot
be remapped safely, so quit with Control+Q.

The editor reads word motion from the Option (Alt) key. It accepts the two
common forms that terminals send: the xterm modified-arrow sequence and the
Meta "Option b" / "Option f" sequence. In Terminal.app, enable "Use Option as
Meta key" in the profile keyboard settings so that Option+Left and
Option+Right send word motion.

## Configuration

The editor reads settings from `~/.zerc` at startup. The file is optional; the
editor uses sensible defaults when the file is not there. Each line holds one
setting in the form `key = value`. Lines that start with `#` are comments.
Blank lines and unknown keys are skipped.

```sh
# tab width in spaces (1..16)
tab_size = 4

# save the file after this many edits; 0 turns auto-save off
auto_save = 0

# theme colors as ANSI SGR numbers; leave a value out to keep the terminal color
color_foreground = 37
color_background = 40
color_status_bar = 7
color_line_numbers = 90
color_search_highlight = 33

# rebind a key to a command
bind ctrl+q = quit
bind ctrl+s = save
```

Supported commands for `bind`: `quit`, `save`, `undo`, `redo`, `search`,
`select_all`, `copy`, `cut`, `paste`, `home`, `end`, `page_up`, `page_down`.
Supported key names: `ctrl+a` .. `ctrl+z`, `home`, `end`, `page_up`,
`page_down`, and `delete`. A binding for a key that already has a default
replaces the default.

## Project Structure

```
ze/
├── src/
│   ├── main.c          Entry point
│   ├── editor.c        Editor state and main loop
│   ├── buffer.c        Text buffer (line storage, insert/delete)
│   ├── cursor.c        Cursor movement and bounds checking
│   ├── renderer.c      Screen drawing and scrolling
│   ├── command.c       Command execution
│   ├── keymap.c        Key-to-command translation
│   ├── search.c        Buffer search (match finding, next/prev)
│   ├── selection.c     Text selection state and region operations
│   ├── clipboard.c     Clipboard buffer and paste operation
│   ├── config.c        User configuration (defaults, ~/.zerc parser)
│   └── undo.c          Undo/redo history
├── include/
│   ├── keys.h          Centralized key code definitions
│   ├── command.h       Command types and execution
│   ├── keymap.h        Key translation interface
│   ├── config.h        Config struct, theme, and key bindings
│   ├── undo.h          Undo/redo history interface
│   └── ...             Buffer, cursor, renderer, platform headers
├── platforms/
│   ├── unix/           Unix terminal and filesystem backend
│   └── ginnos/         ginnOS backend (future)
├── tests/
├── docs/
└── Makefile
```

## Roadmap

See [ROADMAP.md](ROADMAP.md) for the full development plan. Current status:

- [x] Phase 1 — Terminal Layer
- [x] Phase 2 — Text Buffer
- [x] Phase 3 — Cursor and Navigation
- [x] Phase 4 — Rendering System
- [x] Phase 5 — Filesystem Abstraction
- [x] Phase 6 — Editor Commands
- [x] Phase 7 — Undo and Redo
- [x] Phase 8 — Search
- [x] Phase 9 — Text Selection
- [x] Phase 10 — Clipboard (Copy / Cut / Paste)
- [x] Phase 11 — Configuration
- [ ] Phase 12 — Syntax Highlighting

## Contributing

See [docs/contributing.md](docs/contributing.md) for coding style, documentation rules, and workflow guidelines. For a deeper look at how the codebase is structured, see [docs/architecture.md](docs/architecture.md).

## (Un)license

This project is part of the ginnOS ecosystem, which all shares the UNLICENSE.
