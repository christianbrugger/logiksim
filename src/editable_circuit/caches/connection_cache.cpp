#include "editable_circuit/caches/connection_cache.h"

#include "geometry.h"
#include "layout_calculations.h"

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

auto get_and_verify_cache_entry(detail::connection_cache::map_type& map, point_t position,
                                detail::connection_cache::connection_data_t value)
    -> detail::connection_cache::map_type::iterator {
    const auto it = map.find(position);
    if (it == map.end() || it->second != value) [[unlikely]] {
        throw_exception("unable to find chached data that should be present.");
    }
    return it;
}

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

auto get_add_entry(detail::connection_cache::map_type& map, element_id_t element_id,
                   segment_index_t segment_index = null_segment_index) {
    return
        [&map, element_id, segment_index](connection_id_t connection_id, point_t position,
                                          orientation_t orientation) -> bool {
            if (map.contains(position)) [[unlikely]] {
                throw_exception("cache already has an entry at this position");
            }
            map[position] = {element_id, segment_index, connection_id, orientation};
            return true;
        };
}

auto get_update_entry(detail::connection_cache::map_type& map,
                      element_id_t new_element_id, element_id_t old_element_id,
                      segment_index_t new_segment_index = null_segment_index,
                      segment_index_t old_segment_index = null_segment_index) {
    return [&map, new_element_id, old_element_id, new_segment_index, old_segment_index](
               connection_id_t connection_id, point_t position,
               orientation_t orientation) -> bool {
        const auto old_value = detail::connection_cache::connection_data_t {
            old_element_id, old_segment_index, connection_id, orientation};
        const auto it = get_and_verify_cache_entry(map, position, old_value);
        it->second.element_id = new_element_id;
        it->second.segment_index = new_segment_index;
        return true;
    };
}

auto get_remove_entry(detail::connection_cache::map_type& map, element_id_t element_id,
                      segment_index_t segment_index = null_segment_index) {
    return
        [&map, element_id, segment_index](connection_id_t connection_id, point_t position,
                                          orientation_t orientation) -> bool {
            const auto value = detail::connection_cache::connection_data_t {
                element_id, segment_index, connection_id, orientation};
            const auto it = get_and_verify_cache_entry(map, position, value);
            map.erase(it);
            return true;
        };
}

template <bool IsInput>
auto ConnectionCache<IsInput>::handle(
    editable_circuit::info_message::LogicItemInserted message) -> void {
    const auto add_entry = get_add_entry(map_, message.element_id);

    if constexpr (IsInput) {
        iter_input_location_and_id(message.data, add_entry);
    } else {
        iter_output_location_and_id(message.data, add_entry);
    }
}

template <bool IsInput>
auto ConnectionCache<IsInput>::handle(
    editable_circuit::info_message::InsertedLogicItemIdUpdated message) -> void {
    const auto update_entry
        = get_update_entry(map_, message.new_element_id, message.old_element_id);

    if constexpr (IsInput) {
        iter_input_location_and_id(message.data, update_entry);
    } else {
        iter_output_location_and_id(message.data, update_entry);
    }
}

template <bool IsInput>
auto ConnectionCache<IsInput>::handle(
    editable_circuit::info_message::LogicItemUninserted message) -> void {
    const auto remove_entry = get_remove_entry(map_, message.element_id);

    if constexpr (IsInput) {
        iter_input_location_and_id(message.data, remove_entry);
    } else {
        iter_output_location_and_id(message.data, remove_entry);
    }
}

// next_connection(connection_id_t input_id, point_t position,
//                 orientation_t orientation) -> bool;
template <typename Func>
auto iter_connection_location_and_id(segment_info_t segment_info, Func next_connection,
                                     bool input) -> bool {
    const auto point_type = input ? SegmentPointType::input : SegmentPointType::output;
    const auto line = segment_info.line;

    if (segment_info.p0_type == point_type) {
        const auto orientation = to_orientation_p0(line);

        if (!next_connection(null_connection, line.p0, orientation)) {
            return false;
        }
    }
    if (segment_info.p1_type == point_type) {
        const auto orientation = to_orientation_p1(line);

        if (!next_connection(null_connection, line.p1, orientation)) {
            return false;
        }
    }
    return true;
}

template <bool IsInput>
auto ConnectionCache<IsInput>::handle(
    editable_circuit::info_message::SegmentInserted message) -> void {
    const auto add_entry
        = get_add_entry(map_, message.segment.element_id, message.segment.segment_index);
    iter_connection_location_and_id(message.segment_info, add_entry, IsInput);
}

template <bool IsInput>
auto ConnectionCache<IsInput>::handle(
    editable_circuit::info_message::InsertedSegmentIdUpdated message) -> void {
    if (message.new_segment == message.old_segment) {
        return;
    }

    const auto update_entry = get_update_entry(
        map_, message.new_segment.element_id, message.old_segment.element_id,
        message.new_segment.segment_index, message.old_segment.segment_index);
    iter_connection_location_and_id(message.segment_info, update_entry, IsInput);
}

template <bool IsInput>
auto ConnectionCache<IsInput>::handle(
    editable_circuit::info_message::InsertedEndPointsUpdated message) -> void {
    using namespace editable_circuit::info_message;

    handle(SegmentUninserted {message.segment, message.old_segment_info});
    handle(SegmentInserted {message.segment, message.new_segment_info});
}

template <bool IsInput>
auto ConnectionCache<IsInput>::handle(
    editable_circuit::info_message::SegmentUninserted message) -> void {
    const auto remove_entry = get_remove_entry(map_, message.segment.element_id,
                                               message.segment.segment_index);
    iter_connection_location_and_id(message.segment_info, remove_entry, IsInput);
}

template <bool IsInput>
auto ConnectionCache<IsInput>::submit(editable_circuit::InfoMessage message) -> void {
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
auto to_connection_entry(auto&& schematic,
                         detail::connection_cache::connection_data_t entry) {
    const auto connection = to_connection<IsInput>(schematic, entry.connection());
    return std::make_pair(connection, entry.orientation);
}

template <bool IsInput>
auto find_impl(const ConnectionCache<IsInput>& cache, point_t position,
               auto&& schematic) {
    if (auto entry = cache.find(position)) {
        return std::optional {to_connection_entry<IsInput>(schematic, entry.value())};
    }

    // nullopt with correct type
    using entry_result_t = decltype(to_connection_entry<IsInput>(
        schematic, std::declval<detail::connection_cache::connection_data_t>()));
    return std::optional<entry_result_t> {};
}

template <bool IsInput>
auto ConnectionCache<IsInput>::find(point_t position, Schematic& schematic) const
    -> std::optional<std::pair<connection_proxy, orientation_t>> {
    return find_impl(*this, position, schematic);
}

template <bool IsInput>
auto ConnectionCache<IsInput>::find(point_t position, const Schematic& schematic) const
    -> std::optional<std::pair<const_connection_proxy, orientation_t>> {
    return find_impl(*this, position, schematic);
}

template <bool IsInput>
auto ConnectionCache<IsInput>::is_colliding(layout_calculation_data_t data) const
    -> bool {
    // make sure inputs/outputs don't collide with inputs/outputs
    const auto same_type_not_colliding
        = [&](point_t position, orientation_t _ [[maybe_unused]]) -> bool {
        return !map_.contains(position);
    };

    // make sure inputs match with output orientations, if present
    const auto different_type_compatible
        = [&](point_t position, orientation_t orientation) -> bool {
        if (const auto it = map_.find(position); it != map_.end()) {
            return orientations_compatible(orientation, it->second.orientation);
        }
        return true;
    };

    if constexpr (IsInput) {
        return !(iter_input_location(data, same_type_not_colliding)
                 && iter_output_location(data, different_type_compatible));
    } else {
        return !(iter_output_location(data, same_type_not_colliding)
                 && iter_input_location(data, different_type_compatible));
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
