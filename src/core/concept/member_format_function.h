#ifndef LOGICSIM_CONCEPT_MEMBER_FORMAT_FUNCTION_H
#define LOGICSIM_CONCEPT_MEMBER_FORMAT_FUNCTION_H

#include "core/concept/string_view.h"

#include <concepts>
#include <string>
#include <string_view>

namespace logicsim {

template <typename T, typename Char = char>
concept has_member_format_function = !string_view<T, Char> && requires(T obj) {
    { obj.format() } -> std::convertible_to<std::string>;
};

}  // namespace logicsim

#endif
