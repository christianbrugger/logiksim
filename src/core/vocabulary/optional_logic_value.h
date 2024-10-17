#ifndef LOGICSIM_VOCABULARY_OPTIONAL_LOGIC_VALUE_H
#define LOGICSIM_VOCABULARY_OPTIONAL_LOGIC_VALUE_H

#include "core/format/enum.h"

#include <fmt/core.h>

#include <cstdint>
#include <optional>
#include <stdexcept>

namespace logicsim {

/**
 * @brief: Logic value that might not be available.
 */
using OptionalLogicValue = std::optional<bool>;

template <>
[[nodiscard]] auto format(OptionalLogicValue value) -> std::string;

}  // namespace logicsim

/**
 *@brief Formatter for OptionalLogicValue
 *
 * Note this is specialized so this one is picked instead of the one for optional.
 */
template <typename Char>
struct fmt::formatter<logicsim::OptionalLogicValue, Char> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::OptionalLogicValue &obj,
                       fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}", logicsim::format(obj));
    }
};

#endif
