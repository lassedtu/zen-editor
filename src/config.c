#include "config.h"

#include "keys.h"
#include "platform_fs.h"

#include <stdlib.h>
#include <string.h>

/**
 * @file config.c
 * @brief the configuration defaults, parser, and file load.
 *
 * this file sets the default Config. it reads a "~/.zerc" file and changes the
 * values. the file has one setting on each line. a value line has the form
 * "key = value". a binding line has the form "bind key = command". the parser
 * uses two name tables. one table changes a command name to a CommandType. the
 * other table changes a key name to a key code. the parser skips a line that
 * it does not know. this keeps the current value safe.
 */

/**
 * @struct NameToCommand
 * @brief a link from a command name to a CommandType.
 */
typedef struct
{
    const char *name; // the name in the configuration file
    CommandType type; // the command for the name
} NameToCommand;

/**
 * @struct NameToKey
 * @brief a link from a key name to a key code.
 */
typedef struct
{
    const char *name; // the name in the configuration file
    int key;          // the key code for the name
} NameToKey;

/* the command names that a binding line can use */
static const NameToCommand k_command_names[] = {
    {"quit", CMD_QUIT},
    {"save", CMD_SAVE},
    {"undo", CMD_UNDO},
    {"redo", CMD_REDO},
    {"search", CMD_SEARCH_OPEN},
    {"select_all", CMD_SELECT_ALL},
    {"copy", CMD_COPY},
    {"cut", CMD_CUT},
    {"paste", CMD_PASTE},
    {"home", CMD_HOME},
    {"end", CMD_END},
    {"page_up", CMD_PAGE_UP},
    {"page_down", CMD_PAGE_DOWN},
};

/* the key names that a binding line can use */
static const NameToKey k_key_names[] = {
    {"ctrl+a", KEY_CTRL('a')},
    {"ctrl+b", KEY_CTRL('b')},
    {"ctrl+c", KEY_CTRL('c')},
    {"ctrl+d", KEY_CTRL('d')},
    {"ctrl+e", KEY_CTRL('e')},
    {"ctrl+f", KEY_CTRL('f')},
    {"ctrl+g", KEY_CTRL('g')},
    {"ctrl+k", KEY_CTRL('k')},
    {"ctrl+l", KEY_CTRL('l')},
    {"ctrl+n", KEY_CTRL('n')},
    {"ctrl+o", KEY_CTRL('o')},
    {"ctrl+p", KEY_CTRL('p')},
    {"ctrl+q", KEY_CTRL('q')},
    {"ctrl+r", KEY_CTRL('r')},
    {"ctrl+s", KEY_CTRL('s')},
    {"ctrl+t", KEY_CTRL('t')},
    {"ctrl+u", KEY_CTRL('u')},
    {"ctrl+v", KEY_CTRL('v')},
    {"ctrl+w", KEY_CTRL('w')},
    {"ctrl+x", KEY_CTRL('x')},
    {"ctrl+y", KEY_CTRL('y')},
    {"ctrl+z", KEY_CTRL('z')},
    {"home", KEY_HOME},
    {"end", KEY_END},
    {"page_up", KEY_PAGE_UP},
    {"page_down", KEY_PAGE_DOWN},
    {"delete", KEY_DELETE},
};

/* the default bindings; these match the built-in key map */
static const KeyBinding k_default_bindings[] = {
    {KEY_CTRL('q'), CMD_QUIT},
    {KEY_CTRL('s'), CMD_SAVE},
    {KEY_CTRL('z'), CMD_UNDO},
    {KEY_CTRL('y'), CMD_REDO},
    {KEY_CTRL('f'), CMD_SEARCH_OPEN},
    {KEY_CTRL('a'), CMD_SELECT_ALL},
    {KEY_CTRL('c'), CMD_COPY},
    {KEY_CTRL('x'), CMD_CUT},
    {KEY_CTRL('v'), CMD_PASTE},
};

/**
 * @brief remove spaces and tabs from the start and the end of a string.
 *
 * the function moves the start pointer past the first spaces. then it writes a
 * null byte after the last character that is not a space. the function changes
 * the string in place.
 *
 * @param s pointer to the string to trim.
 * @return pointer to the first character that is not a space.
 */
static char *trim(char *s)
{
    while (*s == ' ' || *s == '\t')
    {
        s++;
    }

    if (*s == '\0')
    {
        return s;
    }

    char *end = s + strlen(s) - 1;
    while (end > s && (*end == ' ' || *end == '\t' ||
                       *end == '\r' || *end == '\n'))
    {
        *end = '\0';
        end--;
    }

    return s;
}

/**
 * @brief find the command for a command name.
 * @param name the command name to look for.
 * @return the command, or CMD_NONE when the name is not known.
 */
static CommandType command_from_name(const char *name)
{
    int count = (int)(sizeof(k_command_names) / sizeof(k_command_names[0]));
    for (int i = 0; i < count; i++)
    {
        if (strcmp(name, k_command_names[i].name) == 0)
        {
            return k_command_names[i].type;
        }
    }
    return CMD_NONE;
}

/**
 * @brief find the key code for a key name.
 * @param name the key name to look for.
 * @return the key code, or -1 when the name is not known.
 */
static int key_from_name(const char *name)
{
    int count = (int)(sizeof(k_key_names) / sizeof(k_key_names[0]));
    for (int i = 0; i < count; i++)
    {
        if (strcmp(name, k_key_names[i].name) == 0)
        {
            return k_key_names[i].key;
        }
    }
    return -1;
}

/**
 * @brief add a binding to the Config or change one that is there.
 *
 * the function looks for the key in the table. it changes the command when the
 * key is there. it adds a new binding when the key is not there and there is
 * space. the function does nothing when the table is full.
 *
 * @param cfg pointer to the Config to change.
 * @param key the key code to bind.
 * @param type the command for the key.
 */
static void config_set_binding(Config *cfg, int key, CommandType type)
{
    for (int i = 0; i < cfg->num_bindings; i++)
    {
        if (cfg->bindings[i].key == key)
        {
            cfg->bindings[i].type = type;
            return;
        }
    }

    if (cfg->num_bindings < ZE_CONFIG_MAX_BINDINGS)
    {
        cfg->bindings[cfg->num_bindings].key = key;
        cfg->bindings[cfg->num_bindings].type = type;
        cfg->num_bindings++;
    }
}

/**
 * @brief copy a color value into a color field.
 *
 * the function copies at most ZE_COLOR_MAX characters. it always writes a null
 * byte at the end.
 *
 * @param dst the color field to write.
 * @param value the color value to copy.
 */
static void set_color(char *dst, const char *value)
{
    strncpy(dst, value, ZE_COLOR_MAX);
    dst[ZE_COLOR_MAX] = '\0';
}

void config_init(Config *cfg)
{
    cfg->tab_size = 4;
    cfg->auto_save_edits = 0;

    cfg->theme.foreground[0] = '\0';
    cfg->theme.background[0] = '\0';
    cfg->theme.status_bar[0] = '\0';
    cfg->theme.line_numbers[0] = '\0';
    cfg->theme.search_highlight[0] = '\0';

    cfg->num_bindings = 0;
    int count = (int)(sizeof(k_default_bindings) / sizeof(k_default_bindings[0]));
    for (int i = 0; i < count; i++)
    {
        config_set_binding(cfg, k_default_bindings[i].key,
                           k_default_bindings[i].type);
    }
}

/**
 * @brief apply a "key = value" setting to the Config.
 *
 * the function reads a numeric value for "tab_size" and "auto_save". it reads a
 * color value for the theme keys. the function does nothing for an unknown key.
 *
 * @param cfg pointer to the Config to change.
 * @param key the key part of the line, without spaces.
 * @param value the value part of the line, without spaces.
 */
static void apply_setting(Config *cfg, const char *key, const char *value)
{
    if (strcmp(key, "tab_size") == 0)
    {
        int n = atoi(value);
        if (n >= 1 && n <= 16)
        {
            cfg->tab_size = n;
        }
    }
    else if (strcmp(key, "auto_save") == 0)
    {
        int n = atoi(value);
        if (n >= 0)
        {
            cfg->auto_save_edits = n;
        }
    }
    else if (strcmp(key, "color_foreground") == 0)
    {
        set_color(cfg->theme.foreground, value);
    }
    else if (strcmp(key, "color_background") == 0)
    {
        set_color(cfg->theme.background, value);
    }
    else if (strcmp(key, "color_status_bar") == 0)
    {
        set_color(cfg->theme.status_bar, value);
    }
    else if (strcmp(key, "color_line_numbers") == 0)
    {
        set_color(cfg->theme.line_numbers, value);
    }
    else if (strcmp(key, "color_search_highlight") == 0)
    {
        set_color(cfg->theme.search_highlight, value);
    }
    /* the function ignores an unknown key */
}

/**
 * @brief apply a "bind key = command" line to the Config.
 *
 * the function reads the key name and the command name. it adds the binding
 * when both names are known. it does nothing when a name is not known.
 *
 * @param cfg pointer to the Config to change.
 * @param key_name the key name from the line.
 * @param command_name the command name from the line.
 */
static void apply_binding(Config *cfg, const char *key_name,
                          const char *command_name)
{
    int key = key_from_name(key_name);
    if (key < 0)
    {
        return;
    }

    CommandType type = command_from_name(command_name);
    if (type == CMD_NONE)
    {
        return;
    }

    config_set_binding(cfg, key, type);
}

/**
 * @brief parse one line of the configuration file.
 *
 * the function skips an empty line and a comment line. a comment line starts
 * with "#". the function splits a normal line at the "=" sign. it sends a
 * "bind ..." line to apply_binding. it sends any other line to apply_setting.
 *
 * @param cfg pointer to the Config to change.
 * @param line the line text; the function can change this text.
 */
static void parse_line(Config *cfg, char *line)
{
    char *text = trim(line);
    if (text[0] == '\0' || text[0] == '#')
    {
        return;
    }

    char *equals = strchr(text, '=');
    if (equals == NULL)
    {
        return;
    }

    *equals = '\0';
    char *left = trim(text);
    char *right = trim(equals + 1);

    /* a binding line starts with the word "bind" and then a key name */
    if (strncmp(left, "bind ", 5) == 0 || strncmp(left, "bind\t", 5) == 0)
    {
        char *key_name = trim(left + 5);
        apply_binding(cfg, key_name, right);
    }
    else
    {
        apply_setting(cfg, left, right);
    }
}

int config_load_file(Config *cfg, const char *path)
{
    int len = 0;
    char *data = platform_fs_read_file(path, &len);
    if (data == NULL)
    {
        return -1;
    }

    /* read the text line by line; a line ends at a newline or at the end */
    int start = 0;
    for (int i = 0; i <= len; i++)
    {
        if (i == len || data[i] == '\n')
        {
            char saved = data[i];
            if (i < len)
            {
                data[i] = '\0';
            }
            parse_line(cfg, data + start);
            if (i < len)
            {
                data[i] = saved;
            }
            start = i + 1;
        }
    }

    free(data);
    return 0;
}

int config_load_default(Config *cfg)
{
    const char *home = getenv("HOME");
    if (home == NULL || home[0] == '\0')
    {
        return -1;
    }

    /* build the path "<home>/.zerc" */
    const char *suffix = "/.zerc";
    int home_len = (int)strlen(home);
    int path_len = home_len + (int)strlen(suffix);
    char *path = malloc((size_t)path_len + 1);
    if (path == NULL)
    {
        return -1;
    }

    strcpy(path, home);
    strcpy(path + home_len, suffix);

    int result = config_load_file(cfg, path);
    free(path);
    return result;
}

CommandType config_command_for_key(const Config *cfg, int key)
{
    for (int i = 0; i < cfg->num_bindings; i++)
    {
        if (cfg->bindings[i].key == key)
        {
            return cfg->bindings[i].type;
        }
    }
    return CMD_NONE;
}
