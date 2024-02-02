#ifndef LOGIKSIM_INDEX_CONNECTION_INDEX_H
#define LOGIKSIM_INDEX_CONNECTION_INDEX_H

#include "format/struct.h"
#include "layout_message_forward.h"
#include "vocabulary/logicitem_connection.h"
#include "vocabulary/orientation.h"
#include "vocabulary/point.h"
#include "vocabulary/segment.h"

#include <ankerl/unordered_dense.h>

#include <optional>
#include <ranges>

namespace logicsim {

class Layout;
struct layout_calculation_data_t;

namespace connection_index {

enum class ContentType { LogicItem, Wire };
enum class DirectionType { Input, Output };

struct wire_value_t {
    segment_t segment;
    orientation_t orientation;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const wire_value_t& other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const wire_value_t& other) const = default;
};

template <ContentType Content>
using value_t = std::conditional_t<Content == ContentType::LogicItem,
                                   logicitem_connection_t, wire_value_t>;

template <ContentType Content>
using map_type = ankerl::unordered_dense::map<point_t, value_t<Content>>;

using logicitem_map_t = map_type<ContentType::LogicItem>;
using wire_map_t = map_type<ContentType::Wire>;

static_assert(sizeof(value_t<ContentType::LogicItem>) == 8);
static_assert(sizeof(value_t<ContentType::Wire>) == 12);

}  // namespace connection_index

/**
 * @brief: Efficiently stores connector positions of Layout elements.
 *
 * Pre-conditions:
 *   + inserted wire segments need to have the correct SegmentPointType
 *   + requires a correct history of messages of element changes
 *
 * Class-invariants:
 *   + The index does not contain duplicate connections of a single type.
 */
template <connection_index::ContentType Content,
          connection_index::DirectionType Direction>
class ConnectionIndex {
   public:
    using ContentType = connection_index::ContentType;
    using DirectionType = connection_index::DirectionType;

    using value_t = connection_index::value_t<Content>;
    using map_type = connection_index::map_type<Content>;

   public:
    [[nodiscard]] explicit ConnectionIndex() = default;
    [[nodiscard]] explicit ConnectionIndex(const Layout& layout);

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    [[nodiscard]] auto operator==(const ConnectionIndex&) const -> bool = default;

    [[nodiscard]] auto find(point_t position) const -> std::optional<value_t>;
    [[nodiscard]] auto is_colliding(const layout_calculation_data_t& data) const -> bool
        requires(Content == ContentType::LogicItem);

    [[nodiscard]] auto positions() const {
        return std::ranges::views::keys(map_);
    }

    [[nodiscard]] auto positions_and_orientations() const {
        return transform_view(map_, [](const map_type::value_type& value) {
            return std::make_pair(value.first, value.second.orientation);
        });
    }

    auto submit(const editable_circuit::InfoMessage& message) -> void;

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

    map_type map_ {};
};

using LogicItemInputIndex = ConnectionIndex<connection_index::ContentType::LogicItem,
                                            connection_index::DirectionType::Input>;
using LogicItemOutputIndex = ConnectionIndex<connection_index::ContentType::LogicItem,
                                             connection_index::DirectionType::Output>;

using WireInputIndex = ConnectionIndex<connection_index::ContentType::Wire,
                                       connection_index::DirectionType::Input>;
using WireOutputIndex = ConnectionIndex<connection_index::ContentType::Wire,
                                        connection_index::DirectionType::Output>;

static_assert(std::regular<LogicItemInputIndex>);
static_assert(std::regular<LogicItemOutputIndex>);
static_assert(std::regular<WireInputIndex>);
static_assert(std::regular<WireOutputIndex>);

}  // namespace logicsim

#endif
