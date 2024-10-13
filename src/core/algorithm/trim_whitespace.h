/**
 * @brief: Remove whitespaces from string_views.
 */

#ifndef LOGICSIM_CORE_ALGORITHM_TRIM_WHITESPACE_H
#define LOGICSIM_CORE_ALGORITHM_TRIM_WHITESPACE_H

#include <string_view>

namespace logicsim {

/**
 * Remove leading whitespace.
 *
 * Returns a string_view with all whitespace removed from the front of sv.
 * Whitespace means any of [' ', '\n', '\r', '\t'].
 */
[[nodiscard]] auto trim_left(std::string_view sv) -> std::string_view;

/**
 * Remove trailing whitespace.
 *
 * Returns a string_view with all whitespace removed from the back of sv.
 * Whitespace means any of [' ', '\n', '\r', '\t'].
 */
[[nodiscard]] auto trim_right(std::string_view sv) -> std::string_view;

/**
 * Remove leading and trailing whitespace.
 *
 * Returns a string_view with all whitespace removed from the back and front of
 * sv. Whitespace means any of [' ', '\n', '\r', '\t'].
 */
[[nodiscard]] auto trim(std::string_view sv) -> std::string_view;

}  // namespace logicsim

#endif
