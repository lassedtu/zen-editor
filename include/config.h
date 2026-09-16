#ifndef ZE_CONFIG_H
#define ZE_CONFIG_H

#include "command.h"

/**
 * @file config.h
 * @brief user configuration for the editor.
 *
 * this file gives the Config structure and its public functions. the Config
 * holds all values that the user can change. these values are the tab size,
 * the auto-save count, the theme colors, and the key bindings. the editor
 * reads a Config with default values first. then the editor reads a file at
 * "~/.zerc" and changes the values from that file. the core owns the Config.
 * the platform layer does not know about it.
 */

/**
 * @def ZE_CONFIG_MAX_BINDINGS
 * @brief the largest number of key bindings that the Config can hold.
 */
#define ZE_CONFIG_MAX_BINDINGS 64

/**
 * @def ZE_COLOR_MAX
 * @brief the largest length of a color string, without the null byte.
 *
 * a color string holds the numeric part of an ANSI Select Graphic Rendition
 * sequence. an example is "31" for red text.
 */
#define ZE_COLOR_MAX 15

/**
 * @struct Theme
 * @brief the colors for the parts of the screen.
 *
 * each field holds the numeric part of an ANSI color sequence as a string.
 * the renderer puts the sequence prefix and suffix around this part. an empty
 * string tells the renderer to use the color of the terminal.
 */
typedef struct
{
    char foreground[ZE_COLOR_MAX + 1];       // color of the text
    char background[ZE_COLOR_MAX + 1];       // color behind the text
    char status_bar[ZE_COLOR_MAX + 1];       // color of the status bar
    char line_numbers[ZE_COLOR_MAX + 1];     // color of the line numbers
    char search_highlight[ZE_COLOR_MAX + 1]; // color of a search match
} Theme;

/**
 * @struct KeyBinding
 * @brief a link from one key code to one command.
 */
typedef struct
{
    int key;          // the key code from the platform layer
    CommandType type; // the command that the key runs
} KeyBinding;

/**
 * @struct Config
 * @brief all values that the user can change.
 */
typedef struct
{
    int tab_size;          // number of spaces for one tab
    int auto_save_edits;   // number of edits before an auto-save; 0 turns it off
    Theme theme;           // the colors for the screen
    KeyBinding bindings[ZE_CONFIG_MAX_BINDINGS]; // the key-to-command table
    int num_bindings;      // number of bindings that are in use
} Config;

/**
 * @brief set a Config to the default values.
 *
 * the default tab size is 4. the default auto-save count is 0, which turns
 * auto-save off. the default theme uses the terminal colors. the default
 * bindings are the built-in key map.
 *
 * @param cfg pointer to the Config to set.
 */
void config_init(Config *cfg);

/**
 * @brief read a configuration file and change the Config values.
 *
 * the function reads the file at the given path. it reads one setting on each
 * line. the format of a value line is "key = value". the format of a binding
 * line is "bind key = command". the function skips empty lines and lines that
 * start with "#". the function skips a line with an unknown key. the function
 * keeps the current value when a line is not correct.
 *
 * @param cfg pointer to the Config to change.
 * @param path the path to the configuration file.
 * @return 0 when the function reads the file, -1 when the file is not there.
 */
int config_load_file(Config *cfg, const char *path);

/**
 * @brief read the configuration file of the user.
 *
 * the function finds the home directory of the user. then it reads the file at
 * "~/.zerc" with config_load_file. the function makes no change when there is
 * no home directory or no file.
 *
 * @param cfg pointer to the Config to change.
 * @return 0 when the function reads a file, -1 when there is no file.
 */
int config_load_default(Config *cfg);

/**
 * @brief find the command for a key code in the Config bindings.
 * @param cfg pointer to the Config to read.
 * @param key the key code to look for.
 * @return the command for the key, or CMD_NONE when there is no binding.
 */
CommandType config_command_for_key(const Config *cfg, int key);

#endif /* ZE_CONFIG_H */
