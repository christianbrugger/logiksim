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

// TODO use element_t, after we remove element_key_t
struct map_key_t {
    element_key_t element_key {null_element_key};
    segment_index_t segment_index {null_segment_index};

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const map_key_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const map_key_t &other) const = default;
};

using policy = folly::small_vector_policy::policy_size_type<uint16_t>;
using map_value_t = folly::small_vector<segment_selection_t, 3, policy>;
using map_pair_t = std::pair<map_key_t, map_value_t>;

static_assert(sizeof(map_key_t) == 16);
static_assert(sizeof(map_value_t) == 14);
static_assert(sizeof(map_pair_t) == 32);

}  // namespace detail::selection
}  // namespace logicsim

template <>
struct ankerl::unordered_dense::hash<logicsim::detail::selection::map_key_t> {
    using is_avalanching = void;

    using object_type = logicsim::detail::selection::map_key_t;

    [[nodiscard]] auto operator()(const object_type &obj) const noexcept -> uint64_t {
        return logicsim::hash_16_byte(static_cast<uint64_t>(obj.element_key.value),
                                      static_cast<uint64_t>(obj.segment_index.value));
    }
};

namespace logicsim {
namespace detail::selection {
using elements_set_t = ankerl::unordered_dense::set<element_key_t>;
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

    auto add_element(element_key_t element_key) -> void;
    auto remove_element(element_key_t element_key) -> void;
    auto toggle_element(element_key_t element_key) -> void;

    auto add_segment(element_key_t element_key, segment_index_t segment_index,
                     segment_selection_t selection) -> void;
    auto remove_segment(element_key_t element_key, segment_index_t segment_index,
                        segment_selection_t selection) -> void;
    auto toggle_segment(element_key_t element_key, segment_index_t segment_index,
                        segment_selection_t selection) -> void;

    [[nodiscard]] auto is_selected(element_key_t element) const -> bool;

    [[nodiscard]] auto selected_elements() const -> std::span<const element_key_t>;
    [[nodiscard]] auto selected_segments() const -> std::span<const segment_pair_t>;
    [[nodiscard]] auto selected_segments(element_key_t element_key,
                                         segment_index_t segment_index) const
        -> std::span<const segment_selection_t>;

   private:
    detail::selection::elements_set_t selected_elements_ {};
    detail::selection::segment_map_t selected_segments_ {};
};

auto swap(Selection &a, Selection &b) noexcept -> void;

}  // namespace logicsim

template <>
auto std::swap(logicsim::Selection &a, logicsim::Selection &b) noexcept -> void;

#endif
