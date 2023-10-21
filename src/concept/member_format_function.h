#ifndef LOGICSIM_CONCEPT_MEMBER_FORMAT_FUNCTION_H
#define LOGICSIM_CONCEPT_MEMBER_FORMAT_FUNCTION_H

#include "concept/string_view.h"

#include <concepts>
#include <string>
#include <string_view>

namespace logicsim {

template <typename T, typename Char = char>
concept has_member_format_function = !string_view<T, Char> && requires(T obj) {
    { obj.format() } -> std::same_as<std::string>;
};

}

#endif
