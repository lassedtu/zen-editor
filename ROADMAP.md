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
                Editor Core
                    |
                    |
        Platform Abstraction Layer
          /                     \
 Linux/macOS Backend       ginnOS Backend
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

## Phase 1: Terminal layer

First thing to build: basic terminal control. Nothing fancy yet, just:

* Clear screen
* Move the cursor
* Write text
* Handle terminal colors
* Read keyboard input
* Handle special keys

Goal here is just that ze can drive the terminal without us hardcoding OS-specific calls all over the place.

## Phase 2: Text buffer

Next, the actual internal representation of the text, this needs to be totally separate from how it gets drawn to the screen.

It needs to:

* Store lines
* Insert characters
* Delete characters
* Split lines
* Merge lines
* Track what's been modified

This buffer basically becomes the source of truth for whatever document is open.

## Phase 3: Cursor and navigation

Cursor logic goes on top of the buffer, not the terminal directly:

* Moving by character
* Moving by line
* Home/end
* Page scrolling
* Keeping the cursor in bounds

## Phase 4: Rendering

Now build the renderer that actually draws the buffer to the screen. It needs to:

* Draw the visible text
* Draw the cursor
* Handle scrolling
* Only redraw what actually changed

Flow is basically: buffer feeds the renderer, renderer feeds the terminal.

```
Text Buffer
      |
 Renderer
      |
 Terminal
```

## Phase 5: Filesystem abstraction

Same idea as the terminal layer, but for files — ze shouldn't call system file APIs directly. Wrap it in a small abstraction:

```
ze
 |
 |
Platform filesystem API
 |
 |
Operating system
```

Basic ops needed: open, read, write. On Linux/macOS this is just normal syscalls; on ginnOS it'll eventually use ginnOS's own syscalls instead.

## Phase 6: Commands

Build out the command system, save, quit, search, etc:

```
CTRL + S
Save file

CTRL + Q
Quit editor

CTRL + F
Search
```

Keyboard input should get translated into these commands rather than us poking at editor state directly.

## Phase 7: Undo / redo

History needs to be command-based — every edit is an operation that can be reversed:

* Insert character
* Delete character
* Insert line
* Remove line

This gets us undo and redo, and sets things up for macros down the line.

## Phase 8: Search

Basic search first:

* Search forward
* Jump between matches
* Highlight results

Later on, maybe faster search algorithms, maybe regex.

## Phase 9: Configuration

User-facing config options:

* Tab size
* Display settings
* Themes
* Key bindings
* Auto-save

## Phase 10: Syntax highlighting

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

Starting languages: C, Assembly, and Markdown — those are the ones that actually matter for ginnOS dev.

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