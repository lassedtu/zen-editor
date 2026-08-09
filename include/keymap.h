#ifndef ZE_KEYMAP_H
#define ZE_KEYMAP_H

#include "command.h"

/**
 * @file keymap.h
 * @brief key-to-command translation layer.
 *
 * this file declares the keymap interface, which translates raw key codes
 * (produced by the platform terminal layer) into abstract Commands that the
 * editor can execute. this decoupling allows key bindings to be remapped
 * without touching command execution logic.
 */

/**
 * @brief translate a raw key code into an editor command.
 * @param key the key code returned by platform_terminal_read_key().
 * @return the corresponding Command. returns CMD_NONE if the key has no binding.
 */
Command keymap_translate(int key);

#endif /* ZE_KEYMAP_H */
