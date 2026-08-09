#ifndef ZE_KEYS_H
#define ZE_KEYS_H

/**
 * @file keys.h
 * @brief centralized key code definitions for the editor.
 *
 * this file defines all key codes used throughout the editor. both the platform
 * layer (which produces key codes) and the keymap layer (which consumes them)
 * include this header as the single source of truth.
 */

/**
 * @brief convert a character to its corresponding control key code.
 * @param k the character to convert (e.g., 'q' for Ctrl+Q).
 */
#define KEY_CTRL(k) ((k) & 0x1f)

/**
 * @enum Key
 * @brief special key codes for non-printable keys.
 *
 * values start at 1000 to avoid conflict with ASCII character codes.
 */
enum Key
{
    KEY_ARROW_UP = 1000,
    KEY_ARROW_DOWN,
    KEY_ARROW_LEFT,
    KEY_ARROW_RIGHT,
    KEY_HOME,
    KEY_END,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,
    KEY_DELETE,
    KEY_BACKSPACE = 127,
    KEY_ENTER = '\r',
    KEY_ESCAPE = '\x1b',
};

#endif /* ZE_KEYS_H */
