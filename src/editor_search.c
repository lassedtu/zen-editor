#include "editor.h"

#include "search.h"
#include "cursor.h"

/**
 * @file editor_search.c
 * @brief search-mode state transitions for the editor.
 *
 * this file drives the interactive search built on top of the platform-free
 * search core in search.c. it owns the Search state stored on the Editor:
 * opening and closing the prompt, editing the query, and moving the cursor
 * between matches. incremental search is handled here too, re-running the
 * query from the saved origin on every keystroke so results update live.
 */

/**
 * @brief move the cursor to a match, clamping it into the buffer bounds.
 * @param ed pointer to the editor state.
 * @param m the match to jump to.
 */
static void jump_to_match(Editor *ed, SearchMatch m)
{
    ed->cursor.row = m.row;
    ed->cursor.col = m.col;
    cursor_clamp(&ed->cursor, ed->buffer);
}

/**
 * @brief re-run the current query from the saved origin (incremental search).
 *
 * searching always restarts from the position the prompt was opened at so the
 * match set does not drift as the user types. an empty query returns the
 * cursor to that origin.
 *
 * @param ed pointer to the editor state.
 */
static void research_from_origin(Editor *ed)
{
    if (ed->search.query_len == 0)
    {
        ed->cursor.row = ed->search.saved_row;
        ed->cursor.col = ed->search.saved_col;
        return;
    }

    SearchMatch m;
    if (search_find_next(ed->buffer, ed->search.query, ed->search.saved_row,
                         ed->search.saved_col, &m))
    {
        jump_to_match(ed, m);
    }
}

void editor_search_open(Editor *ed)
{
    ed->search.active = 1;
    ed->search.query[0] = '\0';
    ed->search.query_len = 0;
    ed->search.saved_row = ed->cursor.row;
    ed->search.saved_col = ed->cursor.col;
    ed->search.saved_scroll = ed->scroll_offset;
}

void editor_search_close(Editor *ed)
{
    ed->search.active = 0;
}

void editor_search_cancel(Editor *ed)
{
    ed->search.active = 0;
    ed->cursor.row = ed->search.saved_row;
    ed->cursor.col = ed->search.saved_col;
    ed->scroll_offset = ed->search.saved_scroll;
    cursor_clamp(&ed->cursor, ed->buffer);
}

void editor_search_input_char(Editor *ed, char c)
{
    if (ed->search.query_len >= ZE_SEARCH_QUERY_MAX)
    {
        return;
    }
    ed->search.query[ed->search.query_len++] = c;
    ed->search.query[ed->search.query_len] = '\0';
    research_from_origin(ed);
}

void editor_search_backspace(Editor *ed)
{
    if (ed->search.query_len == 0)
    {
        return;
    }
    ed->search.query[--ed->search.query_len] = '\0';
    research_from_origin(ed);
}

void editor_search_next(Editor *ed)
{
    if (ed->search.query_len == 0)
    {
        return;
    }

    /* start one column past the cursor so we advance off the current match */
    SearchMatch m;
    if (search_find_next(ed->buffer, ed->search.query, ed->cursor.row,
                         ed->cursor.col + 1, &m))
    {
        jump_to_match(ed, m);
    }
}

void editor_search_prev(Editor *ed)
{
    if (ed->search.query_len == 0)
    {
        return;
    }

    SearchMatch m;
    if (search_find_prev(ed->buffer, ed->search.query, ed->cursor.row,
                         ed->cursor.col, &m))
    {
        jump_to_match(ed, m);
    }
}
