#ifndef LOGIKSIM_EDITABLE_CIRCUIT_H
#define LOGIKSIM_EDITABLE_CIRCUIT_H

#include "iterator_adaptor.h"
#include "layout.h"
#include "layout_calculation_type.h"
#include "schematic.h"
#include "search_tree.h"

#include <ankerl/unordered_dense.h>

#include <vector>

// Tasks:
// * re-rooting line_trees if input/output is connected that doesn't match
// * merge line trees on insert, when colliding
//
// * consider second editable_circuit for insert / copy paste / selection
//   or find other solution
// * implement collision visualization
// * consider segment / area / point based visualization / selection for line-trees

// Done:
// * implement connection orientation
// * adding it to ConnectionCache & using it for making connections & is_colliding

namespace logicsim {

enum class InsertionMode {
    insert_or_discard,
    collisions,
    temporary,
};

namespace detail::connection_cache {
// TODO use struct packing ?
struct value_type {
    element_id_t element_id;
    connection_id_t connection_id;
    orientation_t orientation;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const value_type& other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const value_type& other) const = default;
};

using map_type = ankerl::unordered_dense::map<point_t, value_type>;

}  // namespace detail::connection_cache

template <bool IsInput>
class ConnectionCache {
   public:
    using map_type = detail::connection_cache::map_type;
    using value_type = detail::connection_cache::value_type;

   public:
    using connection_proxy
        = std::conditional_t<IsInput, Schematic::Input, Schematic::Output>;
    using const_connection_proxy
        = std::conditional_t<IsInput, Schematic::ConstInput, Schematic::ConstOutput>;

    auto insert(element_id_t element_id, layout_calculation_data_t data) -> void;
    auto remove(element_id_t element_id, layout_calculation_data_t data) -> void;
    auto update(element_id_t new_element_id, element_id_t old_element_id,
                layout_calculation_data_t data) -> void;

    [[nodiscard]] auto find(point_t position) const
        -> std::optional<std::pair<connection_t, orientation_t>>;
    [[nodiscard]] auto find(point_t position, Schematic& schematic) const
        -> std::optional<std::pair<connection_proxy, orientation_t>>;
    [[nodiscard]] auto find(point_t position, const Schematic& schematic) const
        -> std::optional<std::pair<const_connection_proxy, orientation_t>>;

    [[nodiscard]] auto is_colliding(layout_calculation_data_t data) const -> bool;

    [[nodiscard]] auto positions() const {
        return std::ranges::views::keys(connections_);
    }

    [[nodiscard]] auto positions_and_orientations() const {
        return transform_view(connections_, [](const map_type::value_type& value) {
            return std::make_pair(value.first, value.second.orientation);
        });
    }

    // TODO implement validate

   private:
    map_type connections_ {};
};

class CollisionCache {
   public:
    enum class ItemType {
        element_body,
        element_connection,
        wire_connection,
        wire_horizontal,
        wire_vertical,
        wire_point,

        // only for collisions
        wire_new_unknown_point,
    };

    enum class CacheState {
        element_body,
        element_connection,
        wire_connection,
        wire_horizontal,
        wire_vertical,
        wire_point,

        // inferred states
        wire_crossing,
        element_wire_connection,

        invalid_state,
    };

    struct collision_data_t {
        element_id_t element_id_body {null_element};
        element_id_t element_id_horizontal {null_element};
        element_id_t element_id_vertical {null_element};

        auto operator==(const collision_data_t& other) const -> bool = default;
    };

    constexpr static inline auto connection_tag = element_id_t {-2};
    static_assert(connection_tag != null_element);
    static_assert(connection_tag < element_id_t {0});

    static_assert(std::is_aggregate_v<collision_data_t>);

    using map_type = ankerl::unordered_dense::map<point_t, collision_data_t>;

   public:
    auto insert(element_id_t element_id, layout_calculation_data_t data) -> void;
    auto remove(element_id_t element_id, layout_calculation_data_t data) -> void;
    auto update(element_id_t new_element_id, element_id_t old_element_id,
                layout_calculation_data_t data) -> void;

    auto insert(element_id_t element_id, segment_info_t segment) -> void;
    auto remove(element_id_t element_id, segment_info_t segment) -> void;

    [[nodiscard]] auto is_colliding(layout_calculation_data_t data) const -> bool;
    [[nodiscard]] auto is_colliding(line_t line) const -> bool;

    [[nodiscard]] auto get_first_wire(point_t position) const -> element_id_t;

    // std::tuple<point_t, CollisionState>
    [[nodiscard]] auto states() const {
        return transform_view(map_, [](const map_type::value_type& value) {
            return std::make_tuple(value.first, to_state(value.second));
        });
    }

    // TODO implement validate

   private:
    [[nodiscard]] static auto to_state(collision_data_t data) -> CacheState;
    [[nodiscard]] auto state_colliding(point_t position, ItemType item_type) const
        -> bool;
    [[nodiscard]] auto creates_loop(line_t line) const -> bool;

    map_type map_ {};
};

class ElementKeyStore {
   public:
    using map_to_id_t = ankerl::unordered_dense::map<element_key_t, element_id_t>;
    using map_to_key_t = ankerl::unordered_dense::map<element_id_t, element_key_t>;

   public:
    auto insert(element_id_t element_id) -> element_key_t;
    auto remove(element_id_t element_id) -> void;
    auto update(element_id_t new_element_id, element_id_t old_element_id) -> void;

    [[nodiscard]] auto to_element_id(element_key_t element_key) const -> element_id_t;
    [[nodiscard]] auto to_element_ids(std::span<const element_key_t> element_keys) const
        -> std::vector<element_id_t>;
    [[nodiscard]] auto to_element_key(element_id_t element_id) const -> element_key_t;
    [[nodiscard]] auto to_element_keys(std::span<const element_id_t> element_ids) const
        -> std::vector<element_key_t>;

    [[nodiscard]] auto element_key_valid(element_key_t element_key) const -> bool;

    [[nodiscard]] auto size() const -> std::size_t;

   private:
    auto insert(element_id_t element_id, element_key_t element_key) -> void;

   private:
    element_key_t next_key_ {0};

    map_to_id_t map_to_id_ {};
    map_to_key_t map_to_key_ {};
};

enum class LineSegmentType {
    horizontal_first,
    vertical_first,
};

class EditableCircuit {
   public:
    [[nodiscard]] EditableCircuit(Schematic&& schematic, Layout&& layout);

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto schematic() const noexcept -> const Schematic&;
    [[nodiscard]] auto layout() const noexcept -> const Layout&;

    auto add_inverter_element(point_t position, InsertionMode insertion_mode,
                              orientation_t orientation = orientation_t::right)
        -> element_key_t;
    auto add_standard_element(ElementType type, std::size_t input_count, point_t position,
                              InsertionMode insertion_mode,
                              orientation_t orientation = orientation_t::right)
        -> element_key_t;

    auto add_line_segment(point_t p0, point_t p1, LineSegmentType segment_type,
                          InsertionMode insertion_mode) -> element_key_t;

    // auto add_wire(LineTree&& line_tree) -> element_key_t;

    auto change_insertion_mode(element_key_t element_key,
                               InsertionMode new_insertion_mode) -> bool;
    // moves or deletes element
    auto is_position_valid(element_key_t element_key, int x, int y) const -> bool;
    auto is_position_valid(element_key_t element_key, point_t position) const -> bool;
    auto move_or_delete_element(element_key_t element_key, point_t position) -> bool;

    // swaps the element with last one and deletes it
    auto delete_element(element_key_t element_key) -> void;

    [[nodiscard]] auto to_element_id(element_key_t element_key) const -> element_id_t;
    [[nodiscard]] auto to_element_ids(std::span<const element_key_t> element_keys) const
        -> std::vector<element_id_t>;
    [[nodiscard]] auto to_element_key(element_id_t element_id) const -> element_key_t;
    [[nodiscard]] auto to_element_keys(std::span<const element_id_t> element_ids) const
        -> std::vector<element_key_t>;
    [[nodiscard]] auto element_key_valid(element_key_t element_key) const -> bool;

    [[nodiscard]] auto query_selection(rect_fine_t rect) const
        -> std::vector<element_id_t>;
    [[nodiscard]] auto query_selection(point_fine_t point) const
        -> std::optional<element_id_t>;

    // todo: extract_schematic, extract_layout

    [[nodiscard]] auto input_positions() const {
        return input_connections_.positions();
    };

    [[nodiscard]] auto input_positions_and_orientations() const {
        return input_connections_.positions_and_orientations();
    };

    [[nodiscard]] auto output_positions() const {
        return output_connections_.positions();
    };

    [[nodiscard]] auto output_positions_and_orientations() const {
        return output_connections_.positions_and_orientations();
    };

    [[nodiscard]] auto collision_states() const {
        return collicions_cache_.states();
    };

    [[nodiscard]] auto selection_rects() const {
        return selection_cache_.rects();
    }

    auto validate() -> void;

   private:
    auto add_placeholder_element() -> element_id_t;
    auto add_and_connect_placeholder(Schematic::Output output) -> element_id_t;

    auto add_line_segment(line_t line, InsertionMode insertion_mode) -> element_key_t;
    auto fix_line_segments(point_t position) -> void;
    auto set_segment_point_type(segment_t segment, point_t position,
                                SegmentPointType point_type) -> void;

    auto disconnect_inputs_and_add_placeholders(element_id_t element_id) -> void;
    auto disconnect_outputs_and_remove_placeholders(element_id_t& element_id) -> void;
    auto add_missing_placeholders_for_outputs(element_id_t element_id) -> void;

    // may change the element_id, as element might be deleted
    auto connect_and_cache_element(element_id_t& element_id) -> void;
    auto change_insertion_mode(element_id_t& element_id, InsertionMode new_insertion_mode)
        -> bool;

    auto swap_and_delete_single_element(element_id_t element_id) -> void;
    auto swap_and_delete_multiple_elements(std::span<const element_id_t> element_ids)
        -> void;

    [[nodiscard]] auto is_representable_(layout_calculation_data_t data) const -> bool;
    [[nodiscard]] auto is_colliding(layout_calculation_data_t data) const -> bool;
    [[nodiscard]] auto is_colliding(line_t line) const -> bool;
    [[nodiscard]] auto is_element_cached(element_id_t element_id) const -> bool;

    auto key_insert(element_id_t element_id) -> element_key_t;
    auto key_remove(element_id_t element_id) -> void;
    auto key_update(element_id_t new_element_id, element_id_t old_element_id) -> void;

    auto cache_insert(element_id_t element_id) -> void;
    auto cache_remove(element_id_t element_id) -> void;
    auto cache_update(element_id_t new_element_id, element_id_t old_element_id) -> void;

    auto cache_insert(element_id_t element_id, segment_info_t segment,
                      segment_index_t segment_index) -> void;
    auto cache_remove(element_id_t element_id, segment_info_t segment,
                      segment_index_t segment_index) -> void;

    // TODO Consider using element_key_t for Caches, especially SearchTree,
    // TODO so we don't need costly update for element_trees
    ElementKeyStore element_keys_;
    ConnectionCache<true> input_connections_;
    ConnectionCache<false> output_connections_;
    CollisionCache collicions_cache_;
    SearchTree selection_cache_;

    Schematic schematic_;
    Layout layout_;
};

}  // namespace logicsim

#endif
