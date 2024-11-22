#ifndef LOGICSIM_CORE_STABLE_SELECTION_H
#define LOGICSIM_CORE_STABLE_SELECTION_H

#include "core/part_selection.h"
#include "core/vocabulary/decoration_key.h"
#include "core/vocabulary/logicitem_key.h"
#include "core/vocabulary/segment_key.h"

#include <vector>

namespace logicsim {

class Selection;
class KeyIndex;

using key_part_selection_t = std::pair<segment_key_t, PartSelection>;

/**
 * @brief: A stable selection based on the unchanging keys.
 */
struct StableSelection {
    std::vector<logicitem_key_t> logicitems {};
    std::vector<decoration_key_t> decorations {};
    std::vector<key_part_selection_t> segments {};

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
