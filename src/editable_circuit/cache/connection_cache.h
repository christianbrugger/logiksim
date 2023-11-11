#ifndef LOGIKSIM_EDITABLE_CIRCUIT_CACHE_CONNECTION_CACHE_H
#define LOGIKSIM_EDITABLE_CIRCUIT_CACHE_CONNECTION_CACHE_H

#include "editable_circuit/message_forward.h"
#include "format/struct.h"
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

namespace connection_cache {

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

}  // namespace connection_cache

template <connection_cache::ContentType Content,
          connection_cache::DirectionType Direction>
class ConnectionCache {
   public:
    using ContentType = connection_cache::ContentType;
    using DirectionType = connection_cache::DirectionType;

    using value_t = connection_cache::value_t<Content>;
    using map_type = connection_cache::map_type<Content>;

   public:
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

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

    map_type map_ {};
};

using LogicItemInputCache = ConnectionCache<connection_cache::ContentType::LogicItem,
                                            connection_cache::DirectionType::Input>;
using LogicItemOutputCache = ConnectionCache<connection_cache::ContentType::LogicItem,
                                             connection_cache::DirectionType::Output>;

using WireInputCache = ConnectionCache<connection_cache::ContentType::Wire,
                                       connection_cache::DirectionType::Input>;
using WireOutputCache = ConnectionCache<connection_cache::ContentType::Wire,
                                        connection_cache::DirectionType::Output>;

}  // namespace logicsim

#endif
