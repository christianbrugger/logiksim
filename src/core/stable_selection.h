#ifndef LOGICSIM_CORE_STABLE_SELECTION_H
#define LOGICSIM_CORE_STABLE_SELECTION_H

#include "core/vocabulary/decoration_key.h"

#include <vector>

namespace logicsim {

class Selection;
class KeyIndex;

/**
 * @brief: A stable selection based on the unchanging keys.
 */
struct StableSelection {
    std::vector<decoration_key_t> logicitems {};
    std::vector<decoration_key_t> decorations {};
    std::vector<decoration_key_t> segments {};

    [[nodiscard]] auto operator==(const StableSelection&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;
};

static_assert(std::regular<StableSelection>);

[[nodiscard]] auto to_stable_selection(const Selection& selection,
                                       const KeyIndex& key_index) -> StableSelection;

[[nodiscard]] auto to_selection(const StableSelection& unique_selection,
                                const KeyIndex& key_index) -> Selection;

}  // namespace logicsim

#endif
