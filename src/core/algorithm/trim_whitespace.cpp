/**
 * Note some code was taken from folly library under Apache-2.0.
 * Specifically from this file:
 *
 *     https://github.com/facebook/folly/blob/main/folly/String.cpp
 *
 *  The code was modified to work on std::string_view instaed of folly::StringPiece.
 */

#include "algorithm/trim_whitespace.h"

namespace logicsim {

namespace {

[[nodiscard]] auto is_oddspace(char c) -> bool {
    return c == '\n' || c == '\t' || c == '\r';
}

}  // namespace

auto trim_left(std::string_view sv) -> std::string_view {
    // Spaces other than ' ' characters are less common but should be
    // checked.  This configuration where we loop on the ' '
    // separately from oddspaces was empirically fastest.

    while (true) {
        while (!sv.empty() && sv.front() == ' ') {
            sv.remove_prefix(1);
        }
        if (!sv.empty() && is_oddspace(sv.front())) {
            sv.remove_prefix(1);
            continue;
        }

        return sv;
    }
}

auto trim_right(std::string_view sv) -> std::string_view {
    // Spaces other than ' ' characters are less common but should be
    // checked.  This configuration where we loop on the ' '
    // separately from oddspaces was empirically fastest.

    while (true) {
        while (!sv.empty() && sv.back() == ' ') {
            sv.remove_suffix(1);
        }
        if (!sv.empty() && is_oddspace(sv.back())) {
            sv.remove_suffix(1);
            continue;
        }

        return sv;
    }
}

auto trim(std::string_view sv) -> std::string_view {
    return trim_left(trim_right(sv));
}

}  // namespace logicsim
