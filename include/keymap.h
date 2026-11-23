#ifndef ZE_KEYMAP_H
#define ZE_KEYMAP_H

#include "command.h"
#include "config.h"

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

/**
 * @brief translate a key code with the bindings of the user first.
 *
 * the function reads the Config bindings first. it returns the command from the
 * Config when the key is in the binding table. it uses the built-in key map for
 * a key that is not in the table. this lets the user change the bindings of the
 * named commands and keep the movement and typing keys.
 *
 * @param cfg pointer to the Config with the bindings of the user.
 * @param key the key code returned by platform_terminal_read_key().
 * @return the corresponding Command, or CMD_NONE when the key has no binding.
 */
Command keymap_translate_config(const Config *cfg, int key);

#endif /* ZE_KEYMAP_H */
