> *Being developed with the help of [mashdtu](https://github.com/mashdtu).*

ze is going to be a standalone terminal text editor, written in C. The idea is to build it as a normal portable editor first, something that runs fine on Linux/macOS while we're developing it, but design it from day one so it can eventually run as a native userspace app on ginnOS.

We want to keep it in the Unix philosophy: small, focused, does one thing. It shouldn't be tied to ginnOS directly. The idea is keeping the actual editor logic completely separate from platform-specific things like terminal handling, keyboard input, file access, etc.

So the same code should run in two places:

* Linux/macOS, while we're building it
* ginnOS, once there's a backend for it

---

## Architecture

Split into two halves: the editor core, and a platform abstraction layer underneath it. The core doesn't know or care what OS it's running on, it just talks to the abstraction layer, and that layer has a Linux/macOS backend and (eventually) a ginnOS backend.

```
┌────────────────────────────────────────────────┐
│                  Editor Core                   │
│  (buffer, cursor, renderer, editor loop)       │
├────────────────────────────────────────────────┤
│                 Platform Layer                 │
│  (unix/, ginnos/, windows/ ...)                │
└────────────────────────────────────────────────┘
```

The platform layer is responsible for:

* Terminal rendering
* Keyboard input
* File access
* Memory allocation
* Talking to the system in general

That separation lets us build and use ze now, on a finished OS, and later just swap in a ginnOS backend without having to touch the core editor at all.

---

## Project layout

```
ze/

├── src/
│
│   ├── buffer.c
│   ├── cursor.c
│   ├── renderer.c
│   ├── commands.c
│   ├── undo.c
│   ├── editor.c
│   └── ...
│
├── include/
│
├── platforms/
│
│   ├── unix/
│   ├── ginnOS/
│   └── ...
│
├── tests/
│
└── Makefile
```

---

## Completed
- Terminal layer
- Text buffer
- Cursor and navigation
- Rendering
- Filesystem abstraction
- Commands

---

## Undo / redo

History needs to be command-based: every edit is an operation that can be reversed:

* Insert character
* Delete character
* Insert line
* Remove line

This gets us undo and redo, and sets things up for macros down the line.

### What needs to be implemented

1. **UndoEntry structure**: a struct that records a single reversible operation: what type of edit was made, where (row/col), and the data involved (e.g. the character inserted, or the text that was deleted).

2. **Undo stack**: a dynamically-sized stack of UndoEntry items attached to the Editor state. Every edit command (insert char, delete char, insert newline, delete line) pushes an entry onto this stack.

3. **Redo stack**: a separate stack that receives entries when the user undoes something. The redo stack is cleared whenever a new edit is made (standard undo/redo semantics).

4. **Inverse operations**: each operation type needs a defined inverse: insert ↔ delete, split line ↔ merge line. The undo system calls the inverse when popping from the undo stack.

5. **Group/batch operations**: some logical edits span multiple low-level operations (e.g. backspace at column 0 merges lines: a delete-line + append). These need to be grouped so a single undo reverses the entire logical action.

6. **Key bindings**: add `Ctrl+Z` (undo) and `Ctrl+Y` (redo) to the keymap and command system. New command types: `CMD_UNDO`, `CMD_REDO`.

7. **Memory management**: cap the undo history at a reasonable depth (e.g. 1000 entries) and free the oldest entries when the cap is exceeded.

---

## Search

Basic search first:

* Search forward
* Jump between matches
* Highlight results

Later on, maybe faster search algorithms, maybe regex.

### What needs to be implemented

1. **Search prompt UI**: a mode where `Ctrl+F` opens a one-line input area at the bottom of the screen (reusing or extending the status bar area). The user types a query and presses Enter to search, or Escape to cancel.

2. **Forward search**: scan from the current cursor position forward through the buffer, wrapping at the end. Return the (row, col) of the first match.

3. **Backward search / previous match**: `Ctrl+F` followed by a "next/prev" key (e.g. `Ctrl+N` / `Ctrl+P`, or arrow keys while in search mode) to cycle through all matches in the buffer.

4. **Cursor jump**: when a match is found, move the cursor to the start of the match and adjust scroll offset so it's visible on screen.

5. **Match highlighting**: while search mode is active, highlight all occurrences of the query in the visible portion of the buffer. This requires the renderer to accept a list of highlight ranges and render them with inverted colors or a distinct attribute.

6. **Incremental search (stretch)**: update matches live as the user types each character in the search prompt.

7. **Integration with command system**: new command types: `CMD_SEARCH_OPEN`, `CMD_SEARCH_NEXT`, `CMD_SEARCH_PREV`, `CMD_SEARCH_CLOSE`.

---

## Configuration

User-facing config options:

* Tab size
* Display settings
* Themes
* Key bindings
* Auto-save

### What needs to be implemented

1. **Config structure**: a `Config` struct in the Editor state holding all configurable values with sensible defaults (tab size = 4, no auto-save, default theme).

2. **Config file format**: a simple key-value text format (e.g. `tab_size = 4`, one setting per line). Stored at `~/.zerc` or a platform-appropriate path.

3. **Config parser**: read the config file at startup, parse each line, validate values, and populate the Config struct. Unknown keys are ignored with a warning.

4. **Tab size**: the renderer and cursor logic need to respect a configurable tab width instead of hardcoding a value. Tabs are rendered as spaces of the configured width.

5. **Theme / color settings**: define a small set of color slots (foreground, background, status bar, line numbers, search highlight). The config file maps these to ANSI color values. The renderer reads from the active theme.

6. **Rebindable keys**: allow the config file to remap keys to commands (e.g. `bind ctrl+w = quit`). The keymap layer needs to read from the Config rather than being fully hardcoded.

7. **Auto-save**: an optional timer-based or edit-count-based mechanism that triggers `buffer_save` periodically. Needs a platform-layer timer or a simple "edits since last save" counter checked in the main loop.

8. **Runtime reload (stretch)**: detect changes to the config file and re-apply settings without restarting the editor.

---

## Syntax highlighting

Add this through a separate lexer/parser layer, not baked into the renderer:

```
Text
 |
Lexer
 |
Tokens
 |
Renderer
```

Starting languages: C, Assembly, and Markdown. Those are the ones that actually matter for ginnOS dev.

### What needs to be implemented

1. **Token types**: define a `TokenType` enum covering the common categories: keyword, type, string literal, number, comment, preprocessor directive, punctuation, identifier, and default/plain text.

2. **Token structure**: a struct holding the token type, start column, and length for a single token on a single line. Each line produces an array of tokens.

3. **Lexer interface**: a generic `Lexer` struct/interface with a function pointer `tokenize_line(const char *line, int len, Token *out, int *num_tokens)`. Each language provides its own implementation of this function.

4. **C lexer**: recognizes C keywords, types, string/char literals, single-line and multi-line comments, preprocessor directives (`#include`, `#define`, etc.), and numeric constants.

5. **Assembly lexer**: recognizes labels, mnemonics, registers, directives (`.section`, `.global`), comments (`;` or platform-specific), and numeric/hex literals.

6. **Markdown lexer**: recognizes headings (`#`), bold/italic markers, code spans/blocks, links, and list markers.

7. **Language detection**: determine which lexer to use based on the file extension of the opened file. Fall back to no highlighting if unrecognized.

8. **Renderer integration**: the renderer receives the token array for each visible line and applies ANSI color codes (from the active theme) around each token span. This must not allocate per-frame; tokenization should be cached per line and invalidated on edit.

9. **Multi-line state**: handle constructs that span lines (e.g. C multi-line comments, markdown code blocks). Each line's lexer call receives a "state entering this line" value so it can continue a previously-opened comment or block.

10. **Performance**: only re-tokenize lines that have been modified. Store a token cache per line in the buffer, invalidated by any edit to that line (or a state change from the line above).

---

## ginnOS integration

Once ze is solid on a normal OS, build the actual ginnOS backend. For that, ginnOS needs to support:

* Userspace execution
* Process loading
* Syscalls
* A terminal interface
* Filesystem access
* Memory allocation

The whole point of the architecture is that this step should only mean writing a new platform layer, the editor core itself shouldn't need to change at all.

### What needs to be implemented

1. **Platform backend scaffolding**: create `platforms/ginnos/` with the same interface as the unix backend: `platform_init`, `platform_cleanup`, `platform_read_key`, `platform_get_terminal_size`, `platform_write`, etc.

2. **Terminal I/O via ginnOS syscalls**: implement terminal read/write using whatever syscall interface ginnOS exposes (likely custom `sys_read`/`sys_write` on a TTY device). Handle raw mode equivalent if the ginnOS terminal supports it.

3. **Keyboard input**: translate ginnOS scancodes or input events into the `Key` enum values defined in `keys.h`. This is the equivalent of the Unix escape sequence parsing but for the ginnOS input model.

4. **Filesystem access**: implement `buffer_load`/`buffer_save` (or the platform file abstraction) using ginnOS filesystem syscalls (`sys_open`, `sys_read`, `sys_write`, `sys_close`).

5. **Memory allocation**: if ginnOS doesn't provide a full libc `malloc`, implement or port a simple allocator on top of `sys_brk` or whatever memory primitives are available.

6. **Build system integration**: add a `make PLATFORM=ginnos` target that compiles with the ginnOS backend, links against the ginnOS C runtime (or freestanding), and produces a flat binary or ELF that the ginnOS loader can execute.

7. **Testing on ginnOS**: once the backend compiles, verify basic flow: launch -> render empty buffer -> type characters -> save -> quit. Then work through edge cases (terminal resize, large files, etc.).

---

## Where this ends up

End goal is a ze that's:

* A native ginnOS app
* Terminal-based
* Able to handle multiple files
* Syntax highlighting
* Configurable
* Undo/redo
* Search
* Plugged into the ginnOS dev environment

Really it's two things at once: an actual useful editor, and a stress test for whether the ginnOS userspace architecture actually holds up.