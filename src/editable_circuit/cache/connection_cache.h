#ifndef LOGIKSIM_EDITABLE_CIRCUIT_CACHE_CONNECTION_CACHE_H
#define LOGIKSIM_EDITABLE_CIRCUIT_CACHE_CONNECTION_CACHE_H

#include "editable_circuit/message_forward.h"
#include "format/struct.h"
#include "vocabulary.h"

#include <ankerl/unordered_dense.h>

#include <ranges>

namespace logicsim {

class Layout;
struct layout_calculation_data_t;

namespace detail::connection_cache {

struct connection_data_t {
    element_id_t element_id;
    segment_index_t segment_index;
    connection_id_t connection_id;
    orientation_t orientation;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto is_connection() const -> bool;
    [[nodiscard]] auto is_wire_segment() const -> bool;

    [[nodiscard]] auto connection() const -> connection_t;
    [[nodiscard]] auto segment() const -> segment_t;

    [[nodiscard]] auto operator==(const connection_data_t& other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const connection_data_t& other) const = default;
};

static_assert(sizeof(connection_data_t) == 12);

using map_type = ankerl::unordered_dense::map<point_t, connection_data_t>;

}  // namespace detail::connection_cache

template <bool IsInput>
class ConnectionCache {
   public:
    using connection_data_t = detail::connection_cache::connection_data_t;
    using map_type = detail::connection_cache::map_type;

   public:
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    [[nodiscard]] auto find(point_t position) const -> std::optional<connection_data_t>;

    [[nodiscard]] auto is_colliding(layout_calculation_data_t data) const -> bool;
    [[nodiscard]] auto is_colliding(point_t position, orientation_t orientation) const
        -> bool;

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

}  // namespace logicsim

#endif
