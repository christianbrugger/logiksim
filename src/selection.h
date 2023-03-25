#ifndef LOGIKSIM_SELECTION_H
#define LOGIKSIM_SELECTION_H

#include "hashing.h"
#include "vocabulary.h"

#include <ankerl/unordered_dense.h>
#include <boost/container/vector.hpp>
#include <folly/small_vector.h>

namespace logicsim {

struct segment_selection_t {
    grid_t begin;
    grid_t end;
};

namespace detail::selection {

struct map_key_t {
    element_key_t element_key {null_element_key};
    segment_index_t segment_index {null_segment_index};
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
        return logicsim::hash_16_byte(obj.element_key.value, obj.segment_index.value);
    }
};

namespace logicsim {
namespace detail::selection {
using elements_set_t = ankerl::unordered_dense::set<element_key_t>;
using segment_map_t = ankerl::unordered_dense::map<map_key_t, map_value_t>;
}  // namespace detail::selection

class Selection {
   public:
    auto add_element(element_key_t element) -> void;
    auto remove_element(element_key_t element) -> void;

    auto add_segment(element_key_t element, segment_index_t segment_index,
                     segment_selection_t selection) -> void;
    auto remove_segment(element_key_t element, segment_index_t segment_index,
                        segment_selection_t selection) -> void;

    [[nodiscard]] auto is_selected(element_key_t element) const -> bool;

   private:
    detail::selection::elements_set_t selected_elements_ {};
    detail::selection::segment_map_t selected_segments_ {};
};

}  // namespace logicsim

#endif
