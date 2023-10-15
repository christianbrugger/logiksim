#include "editable_circuit/cache/connection_cache.h"

#include "allocated_size/ankerl_unordered_dense.h"
#include "allocated_size/trait.h"
#include "editable_circuit/cache/helper.h"
#include "editable_circuit/message.h"
#include "exception.h"
#include "format/container.h"
#include "format/std_type.h"
#include "geometry/orientation.h"
#include "layout_info_iter.h"

namespace logicsim {

//
// connection_data_t
//

namespace detail::connection_cache {
auto connection_data_t::format() const -> std::string {
    return fmt::format("<{}, {}, {}, {}>", element_id, segment_index, connection_id,
                       orientation);
}

auto connection_data_t::is_connection() const -> bool {
    return element_id && connection_id && !segment_index;
}

auto connection_data_t::is_wire_segment() const -> bool {
    return element_id && !connection_id && segment_index;
}

auto connection_cache::connection_data_t::connection() const -> connection_t {
    if (!is_connection()) {
        throw_exception("entry is not a valid connection");
    }
    return connection_t {element_id, connection_id};
}

auto connection_cache::connection_data_t::segment() const -> segment_t {
    if (!is_wire_segment()) {
        throw_exception("entry is not a valid wire segment");
    }
    return segment_t {element_id, segment_index};
}
}  // namespace detail::connection_cache

//
// ConnectionCache
//

template <bool IsInput>
auto ConnectionCache<IsInput>::format() const -> std::string {
    if constexpr (IsInput) {
        return fmt::format("InputCache = {}", map_);
    } else {
        return fmt::format("OutputCache = {}", map_);
    }
}

template <bool IsInput>
auto ConnectionCache<IsInput>::allocated_size() const -> std::size_t {
    return get_allocated_size(map_);
}

namespace detail::connection_cache {

namespace {

auto get_and_verify_cache_entry(map_type& map, point_t position, connection_data_t value)
    -> map_type::iterator {
    const auto it = map.find(position);
    if (it == map.end() || it->second != value) [[unlikely]] {
        throw_exception("unable to find chached data that should be present.");
    }
    return it;
}

auto add_entry(map_type& map, point_t position, connection_data_t data) {
    if (map.contains(position)) [[unlikely]] {
        throw_exception("cache already has an entry at this position");
    }
    map[position] = data;
}

auto remove_entry(map_type& map, point_t position, connection_data_t data) {
    const auto it = get_and_verify_cache_entry(map, position, data);
    map.erase(it);
}

auto update_entry(map_type& map, point_t position, connection_data_t old_data,
                  element_id_t new_element_id,
                  segment_index_t new_segment_index = null_segment_index) {
    const auto it = get_and_verify_cache_entry(map, position, old_data);
    it->second.element_id = new_element_id;
    it->second.segment_index = new_segment_index;
}

}  // namespace

}  // namespace detail::connection_cache

template <bool IsInput>
auto ConnectionCache<IsInput>::handle(
    const editable_circuit::info_message::LogicItemInserted& message) -> void {
    using namespace detail::connection_cache;

    if constexpr (IsInput) {
        for (auto info : iter_input_location_and_id(message.data)) {
            add_entry(map_, info.position,
                      connection_data_t {
                          .element_id = message.element_id,
                          .segment_index = null_segment_index,
                          .connection_id = info.input_id,
                          .orientation = info.orientation,
                      });
        }
    } else {
        for (auto info : iter_output_location_and_id(message.data)) {
            add_entry(map_, info.position,
                      connection_data_t {
                          .element_id = message.element_id,
                          .segment_index = null_segment_index,
                          .connection_id = info.output_id,
                          .orientation = info.orientation,
                      });
        }
    }
}

template <bool IsInput>
auto ConnectionCache<IsInput>::handle(
    const editable_circuit::info_message::InsertedLogicItemIdUpdated& message) -> void {
    using namespace detail::connection_cache;

    if constexpr (IsInput) {
        for (auto info : iter_input_location_and_id(message.data)) {
            update_entry(map_, info.position,
                         connection_data_t {
                             .element_id = message.old_element_id,
                             .segment_index = null_segment_index,
                             .connection_id = info.input_id,
                             .orientation = info.orientation,
                         },
                         message.new_element_id);
        }
    } else {
        for (auto info : iter_output_location_and_id(message.data)) {
            update_entry(map_, info.position,
                         connection_data_t {
                             .element_id = message.old_element_id,
                             .segment_index = null_segment_index,
                             .connection_id = info.output_id,
                             .orientation = info.orientation,
                         },
                         message.new_element_id);
        }
    }
}

template <bool IsInput>
auto ConnectionCache<IsInput>::handle(
    const editable_circuit::info_message::LogicItemUninserted& message) -> void {
    using namespace detail::connection_cache;

    if constexpr (IsInput) {
        for (auto info : iter_input_location_and_id(message.data)) {
            remove_entry(map_, info.position,
                         connection_data_t {
                             .element_id = message.element_id,
                             .segment_index = null_segment_index,
                             .connection_id = info.input_id,
                             .orientation = info.orientation,
                         });
        }
    } else {
        for (auto info : iter_output_location_and_id(message.data)) {
            remove_entry(map_, info.position,
                         connection_data_t {
                             .element_id = message.element_id,
                             .segment_index = null_segment_index,
                             .connection_id = info.output_id,
                             .orientation = info.orientation,
                         });
        }
    }
}

template <bool IsInput>
auto ConnectionCache<IsInput>::handle(
    const editable_circuit::info_message::SegmentInserted& message) -> void {
    using namespace detail::connection_cache;

    constexpr static auto point_type =
        IsInput ? SegmentPointType::input : SegmentPointType::output;

    if (message.segment_info.p0_type == point_type) {
        add_entry(map_, message.segment_info.line.p0,
                  connection_data_t {
                      .element_id = message.segment.element_id,
                      .segment_index = message.segment.segment_index,
                      .connection_id = null_connection,
                      .orientation = to_orientation_p0(message.segment_info.line),
                  });
    }
    if (message.segment_info.p1_type == point_type) {
        add_entry(map_, message.segment_info.line.p1,
                  connection_data_t {
                      .element_id = message.segment.element_id,
                      .segment_index = message.segment.segment_index,
                      .connection_id = null_connection,
                      .orientation = to_orientation_p1(message.segment_info.line),
                  });
    }
}

template <bool IsInput>
auto ConnectionCache<IsInput>::handle(
    const editable_circuit::info_message::InsertedSegmentIdUpdated& message) -> void {
    using namespace detail::connection_cache;
    if (message.new_segment == message.old_segment) {
        return;
    }

    constexpr static auto point_type =
        IsInput ? SegmentPointType::input : SegmentPointType::output;

    if (message.segment_info.p0_type == point_type) {
        update_entry(map_, message.segment_info.line.p0,
                     connection_data_t {
                         .element_id = message.old_segment.element_id,
                         .segment_index = message.old_segment.segment_index,
                         .connection_id = null_connection,
                         .orientation = to_orientation_p0(message.segment_info.line),
                     },
                     message.new_segment.element_id,  //
                     message.new_segment.segment_index);
    }
    if (message.segment_info.p1_type == point_type) {
        update_entry(map_, message.segment_info.line.p1,
                     connection_data_t {
                         .element_id = message.old_segment.element_id,
                         .segment_index = message.old_segment.segment_index,
                         .connection_id = null_connection,
                         .orientation = to_orientation_p1(message.segment_info.line),
                     },
                     message.new_segment.element_id,  //
                     message.new_segment.segment_index);
    }
}

template <bool IsInput>
auto ConnectionCache<IsInput>::handle(
    const editable_circuit::info_message::InsertedEndPointsUpdated& message) -> void {
    using namespace editable_circuit::info_message;

    handle(SegmentUninserted {message.segment, message.old_segment_info});
    handle(SegmentInserted {message.segment, message.new_segment_info});
}

template <bool IsInput>
auto ConnectionCache<IsInput>::handle(
    const editable_circuit::info_message::SegmentUninserted& message) -> void {
    using namespace detail::connection_cache;

    constexpr static auto point_type =
        IsInput ? SegmentPointType::input : SegmentPointType::output;

    if (message.segment_info.p0_type == point_type) {
        remove_entry(map_, message.segment_info.line.p0,
                     connection_data_t {
                         .element_id = message.segment.element_id,
                         .segment_index = message.segment.segment_index,
                         .connection_id = null_connection,
                         .orientation = to_orientation_p0(message.segment_info.line),
                     });
    }
    if (message.segment_info.p1_type == point_type) {
        remove_entry(map_, message.segment_info.line.p1,
                     connection_data_t {
                         .element_id = message.segment.element_id,
                         .segment_index = message.segment.segment_index,
                         .connection_id = null_connection,
                         .orientation = to_orientation_p1(message.segment_info.line),
                     });
    }
}

template <bool IsInput>
auto ConnectionCache<IsInput>::submit(const editable_circuit::InfoMessage& message)
    -> void {
    using namespace editable_circuit::info_message;

    // logic items
    if (auto pointer = std::get_if<LogicItemInserted>(&message)) {
        handle(*pointer);
        return;
    }
    if (auto pointer = std::get_if<InsertedLogicItemIdUpdated>(&message)) {
        handle(*pointer);
        return;
    }
    if (auto pointer = std::get_if<LogicItemUninserted>(&message)) {
        handle(*pointer);
        return;
    }

    // segments
    if (auto pointer = std::get_if<SegmentInserted>(&message)) {
        handle(*pointer);
        return;
    }
    if (auto pointer = std::get_if<InsertedSegmentIdUpdated>(&message)) {
        handle(*pointer);
        return;
    }
    if (auto pointer = std::get_if<InsertedEndPointsUpdated>(&message)) {
        handle(*pointer);
        return;
    }
    if (auto pointer = std::get_if<SegmentUninserted>(&message)) {
        handle(*pointer);
        return;
    }
}

template <bool IsInput>
auto ConnectionCache<IsInput>::find(point_t position) const
    -> std::optional<connection_data_t> {
    if (const auto it = map_.find(position); it != map_.end()) {
        return it->second;
    }
    return std::nullopt;
}

template <bool IsInput>
auto ConnectionCache<IsInput>::is_colliding(layout_calculation_data_t data) const
    -> bool {
    const auto same_type_not_colliding = [&](const auto& info) -> bool {
        return !map_.contains(info.position);
    };

    // make sure inputs match with output orientations, if present
    const auto different_type_compatible = [&](const auto& info) -> bool {
        if (const auto it = map_.find(info.position); it != map_.end()) {
            return orientations_compatible(info.orientation, it->second.orientation);
        }
        return true;
    };

    if constexpr (IsInput) {
        return !std::ranges::all_of(iter_input_location(data), same_type_not_colliding) ||
               !std::ranges::all_of(iter_output_location(data),
                                    different_type_compatible);

    } else {
        return !std::ranges::all_of(iter_output_location(data),
                                    same_type_not_colliding) ||
               !std::ranges::all_of(iter_input_location(data), different_type_compatible);
    }
}

template <bool IsInput>
auto ConnectionCache<IsInput>::is_colliding(point_t position,
                                            orientation_t orientation) const -> bool {
    if (const auto it = map_.find(position); it != map_.end()) {
        return !orientations_compatible(orientation, it->second.orientation);
    }
    return false;
}

template <bool IsInput>
auto ConnectionCache<IsInput>::validate(const Layout& layout) const -> void {
    auto cache = ConnectionCache<IsInput> {};
    add_layout_to_cache(cache, layout);

    if (cache.map_ != this->map_) [[unlikely]] {
        throw_exception("current cache state doesn't match circuit");
    }
}

template class ConnectionCache<true>;
template class ConnectionCache<false>;

}  // namespace logicsim
