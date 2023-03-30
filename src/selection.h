#ifndef LOGIKSIM_SELECTION_H
#define LOGIKSIM_SELECTION_H

#include "hashing.h"
#include "vocabulary.h"

#include <ankerl/unordered_dense.h>
#include <boost/container/vector.hpp>
#include <folly/small_vector.h>

namespace logicsim {

// TODO rename
// TODO use offsets instead of absolute values
struct segment_selection_t {
    grid_t begin;
    grid_t end;

    [[nodiscard]] explicit constexpr segment_selection_t(grid_t begin_, grid_t end_)
        : begin {begin_}, end {end_} {
        if (!(begin_ < end_)) [[unlikely]] {
            throw_exception("begin needs to be smaller than end.");
        }
    };

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const segment_selection_t &other) const -> bool
        = default;
    [[nodiscard]] auto operator<=>(const segment_selection_t &other) const = default;
};

namespace detail::selection {

using map_key_t = segment_t;
using policy = folly::small_vector_policy::policy_size_type<uint16_t>;
using map_value_t = folly::small_vector<segment_selection_t, 3, policy>;
using map_pair_t = std::pair<map_key_t, map_value_t>;

static_assert(sizeof(map_key_t) == 8);
static_assert(sizeof(map_value_t) == 14);
static_assert(sizeof(map_pair_t) == 24);

using elements_set_t = ankerl::unordered_dense::set<element_id_t>;
using segment_map_t = ankerl::unordered_dense::map<map_key_t, map_value_t>;
}  // namespace detail::selection

auto get_segment_selection(line_t line) -> segment_selection_t;

auto get_segment_selection(line_t line, rect_fine_t selection_rect)
    -> std::optional<segment_selection_t>;

auto get_selected_segment(line_t segment, segment_selection_t selection) -> line_t;

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

    auto add_segment(segment_t segment, segment_selection_t selection) -> void;
    auto remove_segment(segment_t segment, segment_selection_t selection) -> void;
    auto toggle_segment(segment_t segment, segment_selection_t selection) -> void;

    [[nodiscard]] auto is_selected(element_id_t element_id) const -> bool;

    [[nodiscard]] auto selected_elements() const -> std::span<const element_id_t>;
    [[nodiscard]] auto selected_segments() const -> std::span<const segment_pair_t>;
    [[nodiscard]] auto selected_segments(segment_t segment) const
        -> std::span<const segment_selection_t>;

    auto update_element_id(element_id_t new_element_id, element_id_t old_element_id)
        -> void;
    auto remove_segment(segment_t segment) -> void;
    auto update_segment_id(segment_t new_segment, segment_t old_segment) -> void;

   private:
    detail::selection::elements_set_t selected_elements_ {};
    detail::selection::segment_map_t selected_segments_ {};
};

auto swap(Selection &a, Selection &b) noexcept -> void;

}  // namespace logicsim

template <>
auto std::swap(logicsim::Selection &a, logicsim::Selection &b) noexcept -> void;

namespace logicsim {

class Layout;

auto get_pivot(const Selection &selection, const Layout &layout)
    -> std::optional<point_t>;

}  // namespace logicsim

#endif
