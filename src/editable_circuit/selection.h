#ifndef LOGIKSIM_SELECTION_H
#define LOGIKSIM_SELECTION_H

#include "editable_circuit/messages.h"
#include "hashing.h"
#include "vocabulary.h"

#include <ankerl/unordered_dense.h>
#include <folly/small_vector.h>

namespace logicsim {

class Circuit;

namespace detail::selection {

using map_key_t = segment_t;
using policy = folly::small_vector_policy::policy_size_type<uint16_t>;
using map_value_t = folly::small_vector<segment_part_t, 3, policy>;
using map_pair_t = std::pair<map_key_t, map_value_t>;

static_assert(sizeof(map_key_t) == 8);
static_assert(sizeof(map_value_t) == 14);
static_assert(sizeof(map_pair_t) == 24);

using elements_set_t = ankerl::unordered_dense::set<element_id_t>;
using segment_map_t = ankerl::unordered_dense::map<map_key_t, map_value_t>;
}  // namespace detail::selection

auto get_segment_part(line_t line) -> segment_part_t;

auto get_segment_part(line_t line, rect_fine_t selection_rect)
    -> std::optional<segment_part_t>;

auto get_selected_segment(line_t segment, segment_part_t selection) -> line_t;

class Selection {
   public:
    using segment_pair_t = detail::selection::map_pair_t;

    auto swap(Selection &other) noexcept -> void;
    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto empty() const noexcept -> bool;
    auto clear() -> void;

    auto add_element(element_id_t element_id) -> void;
    auto remove_element(element_id_t element_id) -> void;
    auto toggle_element(element_id_t element_id) -> void;

    auto add_segment(segment_t segment, segment_part_t selection) -> void;
    auto remove_segment(segment_t segment, segment_part_t selection) -> void;
    auto toggle_segment(segment_t segment, segment_part_t selection) -> void;

    [[nodiscard]] auto is_selected(element_id_t element_id) const -> bool;

    [[nodiscard]] auto selected_elements() const -> std::span<const element_id_t>;
    [[nodiscard]] auto selected_segments() const -> std::span<const segment_pair_t>;
    [[nodiscard]] auto selected_segments(segment_t segment) const
        -> std::span<const segment_part_t>;

    auto submit(editable_circuit::InfoMessage message) -> void;
    auto validate(const Circuit &circuit) const -> void;

   private:
    auto handle(editable_circuit::info_message::ElementDeleted message) -> void;
    auto handle(editable_circuit::info_message::ElementUpdated message) -> void;

    auto handle(editable_circuit::info_message::SegmentDeleted message) -> void;
    auto handle(editable_circuit::info_message::SegmentUpdated message) -> void;

    detail::selection::elements_set_t selected_elements_ {};
    detail::selection::segment_map_t selected_segments_ {};
};

auto swap(Selection &a, Selection &b) noexcept -> void;

}  // namespace logicsim

template <>
auto std::swap(logicsim::Selection &a, logicsim::Selection &b) noexcept -> void;

#endif
