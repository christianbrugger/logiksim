#ifndef LOGIKSIM_INDEX_COLLISION_INDEX_H
#define LOGIKSIM_INDEX_COLLISION_INDEX_H

#include "core/format/enum.h"
#include "core/format/struct.h"
#include "core/iterator_adaptor/transform_view.h"
#include "core/layout_message_forward.h"
#include "core/vocabulary/decoration_id.h"
#include "core/vocabulary/logicitem_id.h"
#include "core/vocabulary/point.h"
#include "core/vocabulary/wire_id.h"

#include <ankerl/unordered_dense.h>

#include <concepts>

namespace logicsim {

struct ordered_line_t;
struct layout_calculation_data_t;
struct decoration_layout_data_t;
class Layout;

namespace collision_index {

/**
 * @brief: The type of item when adding a new item at a specific position.
 *
 * Note that some states cannot be inserted into the cache.
 */
enum class ItemType {
    logicitem_body,
    logicitem_connection,
    decoration,
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
 * Note, some states are a combination of multiple items inserted at the same positions.
 */
enum class IndexState {
    logicitem_body,
    logicitem_connection,
    decoration,
    wire_connection,
    wire_horizontal,
    wire_vertical,
    wire_corner_point,
    wire_cross_point,

    // combination states
    wire_crossing,
    logicitem_wire_connection,
};

/**
 * @brief: Indicates element input / output || wire input / output
 *         is at this position.
 */
constexpr static inline auto connection_tag = wire_id_t {-2};
/**
 * @brief: Indicates that a decoration is at this position.
 */
constexpr static inline auto decoration_tag = wire_id_t {-3};
/**
 * @brief: Indicates a element is empty
 */
constexpr static inline auto null_element_tag = int32_t {-1};
/**
 * @brief: Indicates a wire corner is at this position.
 */
constexpr static inline auto wire_corner_point_tag = int32_t {-2};
/**
 * @brief: Indicates a wire cross-point is at this position.
 */
constexpr static inline auto wire_cross_point_tag = int32_t {-3};

static_assert(!bool {connection_tag});
static_assert(connection_tag != null_wire_id);

static_assert(!bool {decoration_tag});
static_assert(decoration_tag != null_wire_id);
static_assert(decoration_tag != connection_tag);

static_assert(logicitem_id_t {null_element_tag} == null_logicitem_id);
static_assert(decoration_id_t {null_element_tag} == null_decoration_id);

static_assert(!bool {logicitem_id_t {wire_corner_point_tag}});
static_assert(!bool {decoration_id_t {wire_corner_point_tag}});
static_assert(logicitem_id_t {wire_corner_point_tag} != null_logicitem_id);
static_assert(decoration_id_t {wire_corner_point_tag} != null_decoration_id);

static_assert(!bool {logicitem_id_t {wire_cross_point_tag}});
static_assert(!bool {decoration_id_t {wire_cross_point_tag}});
static_assert(logicitem_id_t {wire_cross_point_tag} != null_logicitem_id);
static_assert(decoration_id_t {wire_cross_point_tag} != null_decoration_id);
static_assert(wire_cross_point_tag != wire_corner_point_tag);

/**
 * @brief: The stored cache value type.
 *
 * Class Invariants:
 *  - Makes sure collision_data contains one of the states or is empty
 */
class collision_data_t {
   public:
    auto operator==(const collision_data_t& other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
    // TODO delete all tags and re-define empty method
    [[nodiscard]] auto empty() const -> bool;

    [[nodiscard]] auto is_logicitem_body() const -> bool;
    [[nodiscard]] auto is_logicitem_connection() const -> bool;
    [[nodiscard]] auto is_decoration() const -> bool;
    [[nodiscard]] auto is_wire_connection() const -> bool;
    [[nodiscard]] auto is_wire_horizontal() const -> bool;
    [[nodiscard]] auto is_wire_vertical() const -> bool;
    [[nodiscard]] auto is_wire_corner_point() const -> bool;
    [[nodiscard]] auto is_wire_cross_point() const -> bool;
    // inferred states -> two elements
    [[nodiscard]] auto is_wire_crossing() const -> bool;
    [[nodiscard]] auto is_logicitem_wire_connection() const -> bool;

    /**
     * @brief: Converts cache state value to enum type.
     *
     * Precondition: Not to be called on an empty state.
     */
    [[nodiscard]] auto to_state() const -> IndexState;

    /**
     * @brief: Return first wire or null_wire_id.
     */
    [[nodiscard]] auto get_first_wire() const -> wire_id_t;

    auto set_logicitem_state(ItemType item_type, logicitem_id_t verify_old_id,
                             logicitem_id_t set_new_id) -> void;
    auto set_decoration_state(ItemType item_type, decoration_id_t verify_old_id,
                              decoration_id_t set_new_id) -> void;
    auto set_wire_state(ItemType item_type, wire_id_t verify_old_id,
                        wire_id_t set_new_id) -> void;

   private:
    auto set_connection_tag() -> void;
    auto set_decoration_tag() -> void;
    auto set_wire_corner_point_tag() -> void;
    auto set_wire_cross_point_tag() -> void;

   private:
    /**
     * @brief: null_element_tag || logicitem_id || decoration_id
     *         wire_corner_point_tag || wire_cross_point_tag ||
     */
    int32_t element_id_ {null_element_tag};
    /**
     * @brief: horizontal wire || null_wire_id
     */
    wire_id_t wire_id_horizontal_ {null_wire_id};
    /**
     * @brief: vertical wire || connection_tag || null_wire_id
     */
    wire_id_t wire_id_vertical_ {null_wire_id};
};

static_assert(std::regular<collision_data_t>);

using map_type = ankerl::unordered_dense::map<point_t, collision_index::collision_data_t>;
}  // namespace collision_index

template <>
[[nodiscard]] auto format(collision_index::ItemType type) -> std::string;

template <>
[[nodiscard]] auto format(collision_index::IndexState state) -> std::string;

/**
 * @brief: Efficiently store collision information of the Layout
 *
 * Pre-conditions:
 *   + inserted wire segments need to have the correct SegmentPointType
 *   + requires a correct history of messages of element changes
 *
 * Class-invariants:
 *   + inserted wires & logicitems are not colliding
 */
class CollisionIndex {
   public:
    using map_type = collision_index::map_type;

   public:
    [[nodiscard]] explicit CollisionIndex() = default;
    [[nodiscard]] explicit CollisionIndex(const Layout& layout);

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    [[nodiscard]] auto operator==(const CollisionIndex&) const -> bool = default;

    [[nodiscard]] auto is_colliding(const layout_calculation_data_t& data) const -> bool;
    [[nodiscard]] auto is_colliding(const decoration_layout_data_t& data) const -> bool;
    [[nodiscard]] auto is_colliding(ordered_line_t line) const -> bool;
    [[nodiscard]] auto is_wires_crossing(point_t point) const -> bool;
    [[nodiscard]] auto is_wire_cross_point(point_t point) const -> bool;

    [[nodiscard]] auto query(point_t point) const -> collision_index::collision_data_t;

    [[nodiscard]] auto get_first_wire(point_t position) const -> wire_id_t;

    // std::tuple<point_t, CollisionState>
    [[nodiscard]] auto states() const {
        return transform_view(map_, [](const map_type::value_type& value) {
            return std::make_tuple(value.first, value.second.to_state());
        });
    }

    auto submit(const InfoMessage& message) -> void;

   private:
    auto handle(const info_message::LogicItemInserted& message) -> void;
    auto handle(const info_message::InsertedLogicItemIdUpdated& message) -> void;
    auto handle(const info_message::LogicItemUninserted& message) -> void;

    auto handle(const info_message::DecorationInserted& message) -> void;
    auto handle(const info_message::InsertedDecorationIdUpdated& message) -> void;
    auto handle(const info_message::DecorationUninserted& message) -> void;

    auto handle(const info_message::SegmentInserted& message) -> void;
    auto handle(const info_message::InsertedSegmentIdUpdated& message) -> void;
    auto handle(const info_message::InsertedEndPointsUpdated& message) -> void;
    auto handle(const info_message::SegmentUninserted& message) -> void;

    [[nodiscard]] auto state_colliding(point_t position,
                                       collision_index::ItemType item_type) const -> bool;

    map_type map_ {};
};

static_assert(std::regular<CollisionIndex>);

}  // namespace logicsim

#endif
