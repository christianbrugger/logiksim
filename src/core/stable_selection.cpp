#include "core/stable_selection.h"

#include "core/algorithm/to_vector.h"
#include "core/allocated_size/std_pair.h"
#include "core/allocated_size/std_vector.h"
#include "core/format/std_type.h"
#include "core/index/key_index.h"
#include "core/selection.h"

#include <algorithm>

namespace logicsim {

StableSelection::StableSelection(std::vector<logicitem_key_t>&& logicitems,
                                 std::vector<decoration_key_t>&& decorations,
                                 std::vector<key_part_selection_t>&& segments)
    : logicitems_ {std::move(logicitems)},
      decorations_ {std::move(decorations)},
      segments_ {std::move(segments)} {
    std::ranges::sort(logicitems_);
    std::ranges::sort(decorations_);
    std::ranges::sort(segments_);

    const auto to_key = [](const key_part_selection_t& pair) { return pair.first; };

    // ensure no duplicate keys
    if (std::ranges::adjacent_find(logicitems_) != logicitems_.end() ||
        std::ranges::adjacent_find(decorations_) != decorations_.end() ||
        std::ranges::adjacent_find(segments_, {}, to_key) != segments_.end())
        [[unlikely]] {
        throw std::runtime_error("vector contains duplicate keys");
    }
}

auto StableSelection::format() const -> std::string {
    return fmt::format("StableSlection(logicitems = {}, decorations = {}, segments = {})",
                       logicitems_, decorations_, segments_);
}

auto StableSelection::allocated_size() const -> std::size_t {
    return get_allocated_size(logicitems_) +   //
           get_allocated_size(decorations_) +  //
           get_allocated_size(segments_);
}

auto StableSelection::logicitems() const -> std::span<const logicitem_key_t> {
    return logicitems_;
}

auto StableSelection::decorations() const -> std::span<const decoration_key_t> {
    return decorations_;
}

auto StableSelection::segments() const -> std::span<const key_part_selection_t> {
    return segments_;
}

[[nodiscard]] auto to_stable_selection(const Selection& selection,
                                       const KeyIndex& key_index) -> StableSelection {
    const auto to_logicitem_key = [&](const logicitem_id_t& logicitem_id_) {
        return key_index.get(logicitem_id_);
    };
    const auto to_decoration_key = [&](const decoration_id_t& decoration_id_) {
        return key_index.get(decoration_id_);
    };
    const auto to_segment_key = [&](const selection::segment_pair_t& pair) {
        return std::pair {key_index.get(pair.first), pair.second};
    };

    return StableSelection {
        to_vector(selection.selected_logicitems() |
                  std::ranges::views::transform(to_logicitem_key)),
        to_vector(selection.selected_decorations() |
                  std::ranges::views::transform(to_decoration_key)),
        to_vector(selection.selected_segments() |
                  std::ranges::views::transform(to_segment_key)),
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
    const auto to_segment_id = [&](const key_part_selection_t& pair) {
        return std::pair {key_index.get(pair.first), pair.second};
    };

    const auto logicitem_ids =
        unique_selection.logicitems() | std::ranges::views::transform(to_logicitem_id);

    const auto decoration_ids =
        unique_selection.decorations() | std::ranges::views::transform(to_decoration_id);

    const auto segment_ids =
        unique_selection.segments() | std::ranges::views::transform(to_segment_id);

    return Selection {
        selection::logicitems_set_t {logicitem_ids.begin(), logicitem_ids.end(),
                                     logicitem_ids.size()},
        selection::decorations_set_t {decoration_ids.begin(), decoration_ids.end(),
                                      decoration_ids.size()},
        selection::segment_map_t {segment_ids.begin(), segment_ids.end(),
                                  segment_ids.size()},
    };
}

}  // namespace logicsim
