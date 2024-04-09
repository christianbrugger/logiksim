#ifndef LOGICSIM_VOCABULARY_DISPLAY_STATE_MAP_H
#define LOGICSIM_VOCABULARY_DISPLAY_STATE_MAP_H

#include "container/enum_map.h"
#include "format/container.h"
#include "vocabulary/display_state.h"

#include <fmt/core.h>

#include <algorithm>
#include <vector>

namespace logicsim {

using DisplayStateMap = EnumMap<display_state_t, all_display_states.size(), bool>;

}

template <typename Char>
struct fmt::formatter<logicsim::DisplayStateMap, Char> {
    constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    inline auto format(const logicsim::DisplayStateMap &obj, fmt::format_context &ctx) {
        using namespace logicsim;

        auto states = std::vector<display_state_t> {};
        std::ranges::copy_if(all_display_states, std::back_inserter(states),
                             [&](const auto state) { return obj.at(state); });

        return fmt::format_to(ctx.out(), "{}", states);
    }
};

#endif
