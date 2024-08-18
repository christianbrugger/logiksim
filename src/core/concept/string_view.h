#ifndef LOGICSIM_FORMAT_CONCEPT_STRING_VIEW_H
#define LOGICSIM_FORMAT_CONCEPT_STRING_VIEW_H

#include <fmt/core.h>

#include <string_view>

namespace logicsim {

/**
 * @brief Matches for types that are a string view
 */
template <typename T, typename Char = char>
concept string_view = std::same_as<T, fmt::basic_string_view<Char>> ||
                      std::same_as<T, std::basic_string<Char>> ||
                      std::same_as<T, std::basic_string_view<Char>>;

}  // namespace logicsim

#endif
