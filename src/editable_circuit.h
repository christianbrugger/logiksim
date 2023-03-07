#ifndef LOGIKSIM_EDITABLE_CIRCUIT_H
#define LOGIKSIM_EDITABLE_CIRCUIT_H

#include "iterator_adaptor.h"
#include "layout.h"
#include "layout_calculation_type.h"
#include "schematic.h"

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
    enum class CollisionState : uint8_t {
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

    [[nodiscard]] auto is_colliding(layout_calculation_data_t data) const -> bool;

    [[nodiscard]] static auto to_state(collision_data_t data) -> CollisionState;

    // std::tuple<point_t, CollisionState>
    [[nodiscard]] auto states() const {
        return transform_view(map_, [](const map_type::value_type& value) {
            return std::make_tuple(value.first, to_state(value.second));
        });
    }

    // TODO implement validate

   private:
    map_type map_ {};
};  // namespace logicsim

class EditableCircuit {
   public:
    // Make private and move to connection cache class
    using connection_map_t = ankerl::unordered_dense::map<point_t, connection_t>;

   public:
    [[nodiscard]] EditableCircuit(Schematic&& schematic, Layout&& layout);

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto schematic() const noexcept -> const Schematic&;
    [[nodiscard]] auto layout() const noexcept -> const Layout&;

    auto add_inverter_element(point_t position, InsertionMode insertion_mode,
                              orientation_t orientation = orientation_t::right) -> bool;
    auto add_standard_element(ElementType type, std::size_t input_count, point_t position,
                              InsertionMode insertion_mode,
                              orientation_t orientation = orientation_t::right) -> bool;

    auto add_wire(LineTree&& line_tree) -> bool;

    // swaps the element with last one and deletes it
    auto swap_and_delete_element(element_id_t element_id) -> void;

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

   private:
    auto add_placeholder_element() -> element_id_t;
    auto add_missing_placeholders(element_id_t element_id) -> void;

    // invalidates the element_id, as element output placeholders might be deleted
    auto connect_new_element(element_id_t& element_id) -> void;

    auto swap_and_delete_single_element(element_id_t element_id) -> void;
    auto swap_and_delete_multiple_elements(std::span<const element_id_t> element_ids)
        -> void;

    [[nodiscard]] auto is_colliding(layout_calculation_data_t data) const -> bool;

    auto cache_insert(element_id_t element_id) -> void;
    auto cache_remove(element_id_t element_id) -> void;
    auto cache_update(element_id_t new_element_id, element_id_t old_element_id) -> void;

    ConnectionCache<true> input_connections_;
    ConnectionCache<false> output_connections_;
    CollisionCache collicions_cache_;

    Schematic schematic_;
    Layout layout_;
};

}  // namespace logicsim

#endif
