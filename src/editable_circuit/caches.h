
#ifndef LOGIKSIM_EDITABLE_CIRCUIT_CACHES_H
#define LOGIKSIM_EDITABLE_CIRCUIT_CACHES_H

#include "circuit.h"
#include "editable_circuit/messages.h"
#include "editable_circuit/spatial_cache.h"
#include "layout_calculation_type.h"
#include "segment_tree.h"
#include "vocabulary.h"

namespace logicsim {

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

    [[nodiscard]] auto format() const -> std::string;

    auto submit(editable_circuit::InfoMessage message) -> void;

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

    auto validate(const Circuit& circuit) const -> void;

   private:
    auto handle(editable_circuit::info_message::ElementInserted message) -> void;
    auto handle(editable_circuit::info_message::ElementUninserted message) -> void;
    auto handle(editable_circuit::info_message::InsertedElementUpdated message) -> void;

    auto handle(editable_circuit::info_message::SegmentInserted message) -> void;
    auto handle(editable_circuit::info_message::SegmentUninserted message) -> void;

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

        // for collisions not insertions
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
    [[nodiscard]] auto format() const -> std::string;

    auto submit(editable_circuit::InfoMessage message) -> void;

    [[nodiscard]] auto is_colliding(layout_calculation_data_t data) const -> bool;
    [[nodiscard]] auto is_colliding(line_t line) const -> bool;

    [[nodiscard]] auto get_first_wire(point_t position) const -> element_id_t;

    // std::tuple<point_t, CollisionState>
    [[nodiscard]] auto states() const {
        return transform_view(map_, [](const map_type::value_type& value) {
            return std::make_tuple(value.first, to_state(value.second));
        });
    }

    auto validate(const Circuit& circuit) const -> void;

   private:
    auto handle(editable_circuit::info_message::ElementInserted message) -> void;
    auto handle(editable_circuit::info_message::ElementUninserted message) -> void;
    auto handle(editable_circuit::info_message::InsertedElementUpdated message) -> void;

    auto handle(editable_circuit::info_message::SegmentInserted message) -> void;
    auto handle(editable_circuit::info_message::SegmentUninserted message) -> void;
    auto handle(editable_circuit::info_message::InsertedSegmentUpdated message) -> void;

    [[nodiscard]] static auto to_state(collision_data_t data) -> CacheState;
    [[nodiscard]] auto state_colliding(point_t position, ItemType item_type) const
        -> bool;
    [[nodiscard]] auto creates_loop(line_t line) const -> bool;

    map_type map_ {};
};

class CacheProvider {
   public:
    auto add_circuit(const Circuit& circuit) -> void;

    [[nodiscard]] auto format() const -> std::string;
    auto validate(const Circuit& circuit) -> void;

    auto submit(editable_circuit::InfoMessage message) -> void;

    [[nodiscard]] auto query_selection(rect_fine_t rect) const
        -> std::vector<SpatialTree::query_result_t>;
    // TODO move where its needed?
    [[nodiscard]] auto query_selection(point_fine_t point) const
        -> std::optional<element_id_t>;

    [[nodiscard]] auto is_element_colliding(layout_calculation_data_t data) const -> bool;

    [[nodiscard]] auto input_cache() const -> const ConnectionCache<true>&;
    [[nodiscard]] auto output_cache() const -> const ConnectionCache<false>&;
    [[nodiscard]] auto collision_cache() const -> const CollisionCache&;
    [[nodiscard]] auto spatial_cache() const -> const SpatialTree&;

   public:
    // for rendering
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
        return collision_cache_.states();
    };

    [[nodiscard]] auto selection_rects() const {
        return spatial_cache_.rects();
    }

   private:
   private:
    ConnectionCache<true> input_connections_ {};
    ConnectionCache<false> output_connections_ {};
    CollisionCache collision_cache_ {};
    SpatialTree spatial_cache_ {};
};

}  // namespace logicsim

#endif
