#ifndef ZE_SEARCH_H
#define ZE_SEARCH_H

#include "buffer.h"

/**
 * @file search.h
 * @brief text search over a buffer, independent of any platform.
 *
 * this file defines the search interface used by the editor. it locates
 * occurrences of a query string within a Buffer and lets the caller step
 * forward and backward between them with wrap-around. all functions operate
 * purely on buffer contents and (row, col) coordinates, so the core stays
 * free of terminal or platform concerns.
 */

/**
 * @struct SearchMatch
 * @brief a single occurrence of the query within the buffer.
 */
typedef struct
{
    int row; // row index of the match
    int col; // starting column index of the match
    int len; // length of the matched text (equal to the query length)
} SearchMatch;

/**
 * @brief find the first match at or after a given position, wrapping around.
 *
 * scans forward from (from_row, from_col). if no match is found before the end
 * of the buffer, the scan wraps to the top and continues up to the start
 * position. an empty query never matches.
 *
 * @param buf pointer to the buffer to search.
 * @param query the string to look for.
 * @param from_row row index to begin the forward scan from.
 * @param from_col column index to begin the forward scan from.
 * @param out pointer that receives the match on success (may be NULL).
 * @return 1 if a match was found, 0 otherwise.
 */
int search_find_next(const Buffer *buf, const char *query, int from_row,
                     int from_col, SearchMatch *out);

/**
 * @brief find the first match strictly before a given position, wrapping around.
 *
 * scans backward from just before (from_row, from_col). if no match is found
 * before the top of the buffer, the scan wraps to the bottom and continues up
 * to the start position. an empty query never matches.
 *
 * @param buf pointer to the buffer to search.
 * @param query the string to look for.
 * @param from_row row index to begin the backward scan from.
 * @param from_col column index to begin the backward scan from.
 * @param out pointer that receives the match on success (may be NULL).
 * @return 1 if a match was found, 0 otherwise.
 */
int search_find_prev(const Buffer *buf, const char *query, int from_row,
                     int from_col, SearchMatch *out);

/**
 * @brief count all non-overlapping occurrences of a query in a single line.
 * @param line pointer to the line to scan.
 * @param query the string to look for.
 * @return the number of matches found on the line (0 for an empty query).
 */
int search_count_in_line(const Line *line, const char *query);

/**
 * @brief find the next match within a single line at or after a column.
 * @param line pointer to the line to scan.
 * @param query the string to look for.
 * @param from_col column index to begin scanning from.
 * @param out_col pointer that receives the match column on success (may be NULL).
 * @return 1 if a match was found on the line, 0 otherwise.
 */
int search_find_in_line(const Line *line, const char *query, int from_col,
                        int *out_col);

#endif /* ZE_SEARCH_H */
