#ifndef LOGIKSIM_EDITABLE_CIRCUIT_CACHE_COLLISION_CACHE_H
#define LOGIKSIM_EDITABLE_CIRCUIT_CACHE_COLLISION_CACHE_H

#include "editable_circuit/message_forward.h"
#include "format/enum.h"
#include "format/struct.h"
#include "iterator_adaptor/transform_view.h"
#include "vocabulary.h"
#include "vocabulary/logicitem_id.h"
#include "vocabulary/wire_id.h"

#include <ankerl/unordered_dense.h>

namespace logicsim {

class Layout;
struct layout_calculation_data_t;

namespace collision_cache {

/**
 * @brief: The type of item when adding a new item at a specific position.
 *
 * Note that some states cannot be inserted into the cache.
 */
enum class ItemType {
    // TODO rename logicitem_*
    // TODO separate enum for logicitem & wire ?
    element_body,
    element_connection,
    wire_connection,
    wire_horizontal,
    wire_vertical,
    wire_corner_point,
    wire_cross_point,

    // for collisions not insertions
    wire_new_unknown_point,
};

/**
 * @brief: The state of the cache at a specific positions.
 *
 * Note that some states are a combination of multiple items inserted at this positions.
 */
enum class CacheState {
    element_body,
    element_connection,
    wire_connection,
    wire_horizontal,
    wire_vertical,
    wire_corner_point,
    wire_cross_point,

    // combination states
    wire_crossing,
    element_wire_connection,

    // TODO remove this?
    invalid_state,
};

/**
 * @brief: The stored cache value type.
 *
 * For each state a CacheState can be derived.
 */
struct collision_data_t {
    /**
     * @brief: logicitem_id || wire_corner_point_tag || wire_cross_point_tag
     */
    logicitem_id_t logicitem_id_body {null_logicitem_id};
    /**
     * @brief: horizontal wire
     */
    wire_id_t wire_id_horizontal {null_wire_id};
    /**
     * @brief: vertical wire || connection_tag
     */
    wire_id_t wire_id_vertical {null_wire_id};

    auto operator==(const collision_data_t& other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::is_aggregate_v<collision_data_t>);

using map_type = ankerl::unordered_dense::map<point_t, collision_cache::collision_data_t>;

/**
 * @brief: Indicates element input / output || wire input / output
 *         is at this position.
 *
 * Set for:
 *      input_location() / output_location() at this position
 *      ItemType::element_connection
 *
 *      SegmentPointType::input
 *      SegmentPointType::output
 *      ItemType::wire_connection
 */
constexpr static inline auto connection_tag = wire_id_t {-2};
/**
 * @brief: Indicates a wire corner is at this position.
 *
 * Set for:
 *      SegmentPointType::corner_point
 *      ItemType::wire_corner_point
 */
constexpr static inline auto wire_corner_point_tag = logicitem_id_t {-3};
/**
 * @brief: Indicates a wire cross-point is at this position.
 *
 * Set for:
 *      SegmentPointType::cross_point
 *      ItemType::wire_cross_point
 */
constexpr static inline auto wire_cross_point_tag = logicitem_id_t {-4};

static_assert(connection_tag != null_wire_id);
static_assert(connection_tag < wire_id_t {0});
static_assert(wire_corner_point_tag != null_logicitem_id);
static_assert(wire_corner_point_tag < logicitem_id_t {0});
static_assert(wire_cross_point_tag != null_logicitem_id);
static_assert(wire_cross_point_tag < logicitem_id_t {0});

[[nodiscard]] auto is_element_body(collision_data_t data) -> bool;
[[nodiscard]] auto is_element_connection(collision_data_t data) -> bool;
[[nodiscard]] auto is_wire_connection(collision_data_t data) -> bool;
[[nodiscard]] auto is_wire_horizontal(collision_data_t data) -> bool;
[[nodiscard]] auto is_wire_vertical(collision_data_t data) -> bool;
[[nodiscard]] auto is_wire_corner_point(collision_data_t data) -> bool;
[[nodiscard]] auto is_wire_cross_point(collision_data_t data) -> bool;
// inferred states -> two elements
[[nodiscard]] auto is_wire_crossing(collision_data_t data) -> bool;
[[nodiscard]] auto is_element_wire_connection(collision_data_t data) -> bool;

/**
 * @brief: Converts cache state value to enum type.
 */
[[nodiscard]] auto to_state(collision_data_t data) -> CacheState;

}  // namespace collision_cache

template <>
auto format(collision_cache::ItemType type) -> std::string;

template <>
auto format(collision_cache::CacheState state) -> std::string;

class CollisionCache {
   public:
    using map_type = collision_cache::map_type;

   public:
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    [[nodiscard]] auto is_colliding(const layout_calculation_data_t& data) const -> bool;
    [[nodiscard]] auto is_colliding(ordered_line_t line) const -> bool;
    [[nodiscard]] auto is_wires_crossing(point_t point) const -> bool;
    [[nodiscard]] auto is_wire_cross_point(point_t point) const -> bool;

    [[nodiscard]] auto query(point_t point) const -> collision_cache::collision_data_t;

    [[nodiscard]] auto get_first_wire(point_t position) const -> wire_id_t;

    // std::tuple<point_t, CollisionState>
    [[nodiscard]] auto states() const {
        return transform_view(map_, [](const map_type::value_type& value) {
            return std::make_tuple(value.first, to_state(value.second));
        });
    }

    auto submit(const editable_circuit::InfoMessage& message) -> void;
    auto validate(const Layout& layout) const -> void;

   private:
    auto handle(const editable_circuit::info_message::LogicItemInserted& message) -> void;
    auto handle(const editable_circuit::info_message::InsertedLogicItemIdUpdated& message)
        -> void;
    auto handle(const editable_circuit::info_message::LogicItemUninserted& message)
        -> void;

    auto handle(const editable_circuit::info_message::SegmentInserted& message) -> void;
    auto handle(const editable_circuit::info_message::InsertedSegmentIdUpdated& message)
        -> void;
    auto handle(const editable_circuit::info_message::InsertedEndPointsUpdated& message)
        -> void;
    auto handle(const editable_circuit::info_message::SegmentUninserted& message) -> void;

    [[nodiscard]] auto state_colliding(point_t position,
                                       collision_cache::ItemType item_type) const -> bool;

    map_type map_ {};
};

}  // namespace logicsim

#endif
