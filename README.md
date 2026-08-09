# ze (zen Editor)
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
| Home / End | Jump to line start/end  |
| Enter      | Insert new line         |
| Backspace  | Delete character before |
| Delete     | Delete character at     |
| Ctrl+S     | Save file               |
| Ctrl+Q     | Quit                    |

## Project Structure

```
ze/
├── src/
│   ├── main.c          Entry point
│   ├── buffer.c        Text buffer (line storage, insert/delete)
│   ├── cursor.c        Cursor movement and bounds checking
│   ├── renderer.c      Screen drawing and scrolling
│   └── editor.c        Editor state and main loop
├── include/            Public headers and platform interfaces
├── platforms/
│   ├── unix/           Unix terminal and filesystem backend
│   └── ginnos/         ginnOS backend (future)
├── tests/
└── Makefile
```

## Roadmap

See [ROADMAP.md](ROADMAP.md) for the full development plan. Current status:

- [x] Phase 1 — Terminal Layer
- [x] Phase 2 — Text Buffer
- [x] Phase 3 — Cursor and Navigation
- [x] Phase 4 — Rendering System
- [x] Phase 5 — Filesystem Abstraction
- [ ] Phase 6 — Editor Commands
- [ ] Phase 7 — Undo and Redo
- [ ] Phase 8 — Search
- [ ] Phase 9 — Configuration
- [ ] Phase 10 — Syntax Highlighting

## Contributing

See [docs/contributing.md](docs/contributing.md) for coding style, documentation rules, and workflow guidelines. For a deeper look at how the codebase is structured, see [docs/architecture.md](docs/architecture.md).

## (Un)license

This project is part of the ginnOS ecosystem, which all shares the UNLICENSE.
