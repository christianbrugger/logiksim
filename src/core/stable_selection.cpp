#include "core/stable_selection.h"

#include "core/algorithm/to_vector.h"
#include "core/allocated_size/std_vector.h"
#include "core/index/key_index.h"
#include "core/selection.h"

namespace logicsim {

auto StableSelection::format() const -> std::string {
    return fmt::format("StableSlection(logicitems = {}, decorations = {}, segments = {})",
                       logicitems, decorations, segments);
}

auto StableSelection::allocated_size() const -> std::size_t {
    return get_allocated_size(logicitems) +   //
           get_allocated_size(decorations) +  //
           get_allocated_size(segments);
}

[[nodiscard]] auto to_stable_selection(const Selection& selection,
                                       const KeyIndex& key_index) -> StableSelection {
    const auto to_logicitem_key = [&](const logicitem_id_t& logicitem_id_) {
        return key_index.get(logicitem_id_);
    };
    const auto to_decoration_key = [&](const decoration_id_t& decoration_id_) {
        return key_index.get(decoration_id_);
    };

    return StableSelection {
        .logicitems = to_vector(selection.selected_logicitems() |
                                std::ranges::views::transform(to_logicitem_key)),
        .decorations = to_vector(selection.selected_decorations() |
                                 std::ranges::views::transform(to_decoration_key)),
    };
}

[[nodiscard]] auto to_selection(const StableSelection& unique_selection,
                                const KeyIndex& key_index) -> Selection {
    const auto to_logicitem_id = [&](const logicitem_key_t& logicitem_key_) {
        return key_index.get(logicitem_key_);
    };
    const auto to_decoration_id = [&](const decoration_key_t& decoration_key_) {
        return key_index.get(decoration_key_);
    };

    const auto logicitem_ids =
        unique_selection.logicitems | std::ranges::views::transform(to_logicitem_id);

    const auto decoration_ids =
        unique_selection.decorations | std::ranges::views::transform(to_decoration_id);

    return Selection {
        selection::logicitems_set_t {logicitem_ids.begin(), logicitem_ids.end(),
                                     unique_selection.logicitems.size()},
        selection::decorations_set_t {decoration_ids.begin(), decoration_ids.end(),
                                      unique_selection.decorations.size()},
        selection::segment_map_t {},
    };
}

}  // namespace logicsim
