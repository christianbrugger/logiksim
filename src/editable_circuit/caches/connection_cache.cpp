#include "editable_circuit/caches/connection_cache.h"

#include "layout_calculations.h"

namespace logicsim {

namespace detail::connection_cache {
inline auto value_type::format() const -> std::string {
    return fmt::format("<Element {}-{} {}>", connection_id, element_id, orientation);
}
}  // namespace detail::connection_cache

auto get_and_verify_cache_entry(detail::connection_cache::map_type& map, point_t position,
                                detail::connection_cache::value_type value)
    -> detail::connection_cache::map_type::iterator {
    const auto it = map.find(position);
    if (it == map.end() || it->second != value) [[unlikely]] {
        throw_exception("unable to find chached data that should be present.");
    }
    return it;
}

template <bool IsInput>
auto ConnectionCache<IsInput>::format() const -> std::string {
    if constexpr (IsInput) {
        return fmt::format("InputCache = {}\n", connections_);
    } else {
        return fmt::format("OutputCache = {}\n", connections_);
    }
}

template <bool IsInput>
auto ConnectionCache<IsInput>::handle(
    editable_circuit::info_message::LogicItemInserted message) -> void {
    const auto add_position = [&](connection_id_t connection_id, point_t position,
                                  orientation_t orientation) {
        if (connections_.contains(position)) [[unlikely]] {
            throw_exception("cache already has an entry at this position");
        }
        connections_[position] = {message.element_id, connection_id, orientation};
        return true;
    };

    if constexpr (IsInput) {
        iter_input_location_and_id(message.data, add_position);
    } else {
        iter_output_location_and_id(message.data, add_position);
    }
}

template <bool IsInput>
auto ConnectionCache<IsInput>::handle(
    editable_circuit::info_message::LogicItemUninserted message) -> void {
    const auto remove_position = [&](connection_id_t connection_id, point_t position,
                                     orientation_t orientation) {
        const auto value = value_type {message.element_id, connection_id, orientation};
        const auto it = get_and_verify_cache_entry(connections_, position, value);
        connections_.erase(it);
        return true;
    };

    if constexpr (IsInput) {
        iter_input_location_and_id(message.data, remove_position);
    } else {
        iter_output_location_and_id(message.data, remove_position);
    }
}

template <bool IsInput>
auto ConnectionCache<IsInput>::handle(
    editable_circuit::info_message::InsertedLogicItemUpdated message) -> void {
    const auto update_id = [&](connection_id_t connection_id, point_t position,
                               orientation_t orientation) {
        const auto old_value
            = value_type {message.old_element_id, connection_id, orientation};
        const auto it = get_and_verify_cache_entry(connections_, position, old_value);
        it->second.element_id = message.new_element_id;
        return true;
    };

    if constexpr (IsInput) {
        iter_input_location_and_id(message.data, update_id);
    } else {
        iter_output_location_and_id(message.data, update_id);
    }
}

template <bool IsInput>
auto ConnectionCache<IsInput>::handle(
    editable_circuit::info_message::SegmentInserted message) -> void {}

template <bool IsInput>
auto ConnectionCache<IsInput>::handle(
    editable_circuit::info_message::SegmentUninserted message) -> void {}

template <bool IsInput>
auto ConnectionCache<IsInput>::submit(editable_circuit::InfoMessage message) -> void {
    using namespace editable_circuit::info_message;

    if (auto pointer = std::get_if<LogicItemInserted>(&message)) {
        handle(*pointer);
        return;
    }
    if (auto pointer = std::get_if<LogicItemUninserted>(&message)) {
        handle(*pointer);
        return;
    }
    if (auto pointer = std::get_if<InsertedLogicItemUpdated>(&message)) {
        handle(*pointer);
        return;
    }
}

template <bool IsInput>
auto ConnectionCache<IsInput>::find(point_t position) const
    -> std::optional<std::pair<connection_t, orientation_t>> {
    if (const auto it = connections_.find(position); it != connections_.end()) {
        return std::make_pair(
            connection_t {it->second.element_id, it->second.connection_id},
            it->second.orientation);
    }
    return std::nullopt;
}

template <bool IsInput>
auto to_connection_entry(auto&& schematic,
                         std::optional<std::pair<connection_t, orientation_t>> entry) {
    return std::make_optional(
        std::make_pair(to_connection<IsInput>(schematic, entry->first), entry->second));
}

template <bool IsInput>
auto find_impl(const ConnectionCache<IsInput>& cache, point_t position,
               auto&& schematic) {
    if (auto entry = cache.find(position)) {
        return to_connection_entry<IsInput>(schematic, entry);
    }
    // nullopt with correct type
    return decltype(to_connection_entry<IsInput>(schematic, {})) {};
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
        return !connections_.contains(position);
    };

    // make sure inputs match with output orientations, if present
    const auto different_type_compatible
        = [&](point_t position, orientation_t orientation) -> bool {
        if (const auto it = connections_.find(position); it != connections_.end()) {
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
auto ConnectionCache<IsInput>::validate(const Circuit& circuit) const -> void {
    auto cache = ConnectionCache<IsInput> {};
    add_circuit_to_cache(cache, circuit);

    if (cache.connections_ != this->connections_) [[unlikely]] {
        throw_exception("current cache state doesn't match circuit");
    }
}

template class ConnectionCache<true>;
template class ConnectionCache<false>;

}  // namespace logicsim
