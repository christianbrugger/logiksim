#ifndef LOGIKSIM_EDITABLE_CIRCUIT_CACHES_COLLISION_CACHE_H
#define LOGIKSIM_EDITABLE_CIRCUIT_CACHES_COLLISION_CACHE_H

#include "editable_circuit/caches/helpers.h"
#include "editable_circuit/messages.h"

#include <ankerl/unordered_dense.h>

namespace logicsim {

class Circuit;

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
    constexpr static inline auto wire_point_tag = element_id_t {-3};
    static_assert(connection_tag != null_element);
    static_assert(connection_tag < element_id_t {0});
    static_assert(wire_point_tag != null_element);
    static_assert(wire_point_tag < element_id_t {0});

    static_assert(std::is_aggregate_v<collision_data_t>);

    using map_type = ankerl::unordered_dense::map<point_t, collision_data_t>;

   public:
    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto is_colliding(layout_calculation_data_t data) const -> bool;
    [[nodiscard]] auto is_colliding(ordered_line_t line) const -> bool;

    [[nodiscard]] auto get_first_wire(point_t position) const -> element_id_t;

    // std::tuple<point_t, CollisionState>
    [[nodiscard]] auto states() const {
        return transform_view(map_, [](const map_type::value_type& value) {
            return std::make_tuple(value.first, to_state(value.second));
        });
    }

    auto submit(editable_circuit::InfoMessage message) -> void;
    auto validate(const Circuit& circuit) const -> void;

   private:
    auto handle(editable_circuit::info_message::LogicItemInserted message) -> void;
    auto handle(editable_circuit::info_message::InsertedLogicItemIdUpdated message)
        -> void;
    auto handle(editable_circuit::info_message::LogicItemUninserted message) -> void;

    auto handle(editable_circuit::info_message::SegmentInserted message) -> void;
    auto handle(editable_circuit::info_message::InsertedSegmentIdUpdated message) -> void;
    auto handle(editable_circuit::info_message::InsertedEndPointsUpdated message) -> void;
    auto handle(editable_circuit::info_message::SegmentUninserted message) -> void;

    [[nodiscard]] static auto to_state(collision_data_t data) -> CacheState;
    [[nodiscard]] auto state_colliding(point_t position, ItemType item_type) const
        -> bool;
    [[nodiscard]] auto creates_loop(ordered_line_t line) const -> bool;

    map_type map_ {};
};

}  // namespace logicsim

#endif
