#include "editable_circuit.h"

#include "algorithm.h"
#include "exceptions.h"
#include "iterator.h"
#include "layout_calculations.h"

#include <fmt/core.h>

#include <algorithm>

namespace logicsim {

using delete_queue_t = folly::small_vector<element_id_t, 6>;

//
// ConnectionCache
//

auto get_and_verify_cache_entry(ConnectionCache<true>::map_type& map, point_t position,
                                element_id_t element_id, connection_id_t connection_id)
    -> ConnectionCache<true>::map_type::iterator {
    const auto it = map.find(position);
    if (it == map.end() || it->second.element_id != element_id
        || it->second.connection_id != connection_id) [[unlikely]] {
        throw_exception("unable to find chached data that should be present.");
    }
    return it;
}

template <bool IsInput>
auto ConnectionCache<IsInput>::insert(element_id_t element_id, const Schematic& schematic,
                                      const Layout& layout) -> void {
    // placeholders are not cached
    if (schematic.element(element_id).is_placeholder()) {
        return;
    }

    const auto add_position = [&](connection_id_t con_id, point_t position) {
        if (connections_.contains(position)) [[unlikely]] {
            throw_exception("cache already has an entry at this position");
        }
        connections_[position] = {element_id, con_id};
    };

    if constexpr (IsInput) {
        for_each_input_location_and_id(schematic, layout, element_id, add_position);
    } else {
        for_each_output_location_and_id(schematic, layout, element_id, add_position);
    }
}

template <bool IsInput>
auto ConnectionCache<IsInput>::remove(element_id_t element_id, const Schematic& schematic,
                                      const Layout& layout) -> void {
    // placeholders are not cached
    if (schematic.element(element_id).is_placeholder()) {
        return;
    }

    const auto remove_position = [&](connection_id_t con_id, point_t position) {
        auto it = get_and_verify_cache_entry(connections_, position, element_id, con_id);
        connections_.erase(it);
    };

    if constexpr (IsInput) {
        for_each_input_location_and_id(schematic, layout, element_id, remove_position);
    } else {
        for_each_output_location_and_id(schematic, layout, element_id, remove_position);
    }
}

template <bool IsInput>
auto ConnectionCache<IsInput>::update(element_id_t new_element_id,
                                      element_id_t old_element_id,
                                      const Schematic& schematic, const Layout& layout)
    -> void {
    // placeholders are not cached
    if (schematic.element(new_element_id).is_placeholder()) {
        return;
    }

    const auto update_id = [&](connection_id_t connection_id, point_t position) {
        auto it = get_and_verify_cache_entry(connections_, position, old_element_id,
                                             connection_id);
        it->second.element_id = new_element_id;
    };

    if constexpr (IsInput) {
        for_each_input_location_and_id(schematic, layout, new_element_id, update_id);
    } else {
        for_each_output_location_and_id(schematic, layout, new_element_id, update_id);
    }
}

template <bool IsInput>
auto ConnectionCache<IsInput>::find(point_t position) const
    -> std::optional<connection_t> {
    if (const auto it = connections_.find(position); it != connections_.end()) {
        return {it->second};
    }
    return std::nullopt;
}

template <bool IsInput>
auto find_impl(const ConnectionCache<IsInput>& cache, point_t position,
               auto&& schematic) {
    if (auto res = cache.find(position)) {
        return std::make_optional(to_connection<IsInput>(schematic, *res));
    }
    // nullopt with correct type
    return std::optional<decltype(to_connection<IsInput>(schematic, {}))> {};
}

template <bool IsInput>
auto ConnectionCache<IsInput>::find(point_t position, Schematic& schematic) const
    -> std::optional<connection_proxy> {
    return find_impl(*this, position, schematic);
}

template <bool IsInput>
auto ConnectionCache<IsInput>::find(point_t position, const Schematic& schematic) const
    -> std::optional<const_connection_proxy> {
    return find_impl(*this, position, schematic);
}

//
// Editable Circuit
//

EditableCircuit::EditableCircuit(Schematic&& schematic, Layout&& layout)
    : schematic_ {std::move(schematic)}, layout_ {std::move(layout)} {}

auto EditableCircuit::format() const -> std::string {
    return fmt::format("EditableCircuit{{\n{}\n{}\n}}", schematic_, layout_);
}

auto EditableCircuit::layout() const noexcept -> const Layout& {
    return layout_;
}

auto EditableCircuit::schematic() const noexcept -> const Schematic& {
    return schematic_;
}

auto EditableCircuit::add_placeholder_element() -> element_id_t {
    constexpr static auto connector_delay
        = delay_t {Schematic::defaults::wire_delay_per_distance.value / 2};

    const auto element_id = layout_.add_placeholder();
    {
        const auto element = schematic_.add_element(Schematic::NewElementData {
            .element_type = ElementType::placeholder,
            .input_count = 1,
            .output_count = 0,
            .history_length = connector_delay,
        });
        if (element.element_id() != element_id) [[unlikely]] {
            throw_exception("Added element ids don't match.");
        }
    }

    return element_id;
}

auto EditableCircuit::add_inverter_element(point_t position,
                                           DisplayOrientation orientation) -> void {
    add_standard_element(ElementType::inverter_element, 1, position, orientation);
}

auto EditableCircuit::add_standard_element(ElementType type, std::size_t input_count,
                                           point_t position,
                                           DisplayOrientation orientation) -> void {
    using enum ElementType;
    if (!(type == and_element || type == or_element || type == xor_element
          || type == inverter_element)) [[unlikely]] {
        throw_exception("The type needs to be a standard element.");
    }
    if (type == inverter_element && input_count != 1) [[unlikely]] {
        throw_exception("Inverter needs to have exactly one input.");
    }
    if (type != inverter_element && input_count < 2) [[unlikely]] {
        throw_exception("Input count needs to be at least 2 for standard elements.");
    }

    auto element_id = layout_.add_logic_element(position, orientation);
    {
        const auto element = schematic_.add_element({
            .element_type = type,
            .input_count = input_count,
            .output_count = 1,
        });
        if (element.element_id() != element_id) [[unlikely]] {
            throw_exception("Added element ids don't match.");
        }
    }
    connect_new_element(element_id);
}

auto EditableCircuit::add_wire(LineTree&& line_tree) -> void {
    auto delays = calculate_output_delays(line_tree);
    auto max_delay = std::ranges::max(delays);

    auto element_id = layout_.add_wire(std::move(line_tree));
    {
        const auto element = schematic_.add_element({
            .element_type = ElementType::wire,
            .input_count = 1,
            .output_count = delays.size(),
            .output_delays = delays,
            .history_length = max_delay,
        });
        if (element.element_id() != element_id) [[unlikely]] {
            throw_exception("Added element ids don't match.");
        }
    }
    connect_new_element(element_id);
}

auto EditableCircuit::swap_and_delete_element(element_id_t element_id) -> void {
    if (schematic_.element(element_id).is_placeholder()) {
        throw_exception("cannot directly delete placeholders.");
    }

    auto delete_queue = delete_queue_t {element_id};

    const auto is_placeholder = [](Schematic::Output output) {
        return output.has_connected_element()
               && output.connected_element().element_type() == ElementType::placeholder;
    };
    transform_if(schematic_.element(element_id).outputs(),
                 std::back_inserter(delete_queue),
                 &Schematic::Output::connected_element_id, is_placeholder);

    swap_and_delete_multiple_elements(delete_queue);
}

auto EditableCircuit::swap_and_delete_multiple_elements(
    std::span<const element_id_t> element_ids) -> void {
    // sort descending, so we don't invalidate our ids
    auto sorted_ids = delete_queue_t {element_ids.begin(), element_ids.end()};
    std::ranges::sort(sorted_ids, std::greater<> {});

    for (const auto element_id : sorted_ids) {
        swap_and_delete_single_element(element_id);
    }
}

auto EditableCircuit::swap_and_delete_single_element(element_id_t element_id) -> void {
    cache_remove(element_id);

    // delete in underlying
    auto last_id1 = schematic_.swap_and_delete_element(element_id);
    auto last_id2 = layout_.swap_and_delete_element(element_id);
    if (last_id1 != last_id2) {
        throw_exception("Returned id's during deletion are not the same.");
    }

    cache_update(element_id, last_id1);
}

auto EditableCircuit::add_missing_placeholders(element_id_t element_id) -> void {
    for (const auto output : schematic_.element(element_id).outputs()) {
        if (!output.has_connected_element()) {
            auto placeholder_id = add_placeholder_element();
            output.connect(schematic_.element(placeholder_id).input(connection_id_t {0}));
        }
    }
}

template <bool IsInput>
auto connect_connector_impl(connection_t connection_data, point_t position,
                            ConnectionCache<IsInput>& connection_cache,
                            Schematic& schematic) -> std::optional<element_id_t> {
    auto unused_placeholder_id = std::optional<element_id_t> {};
    auto connection = to_connection<!IsInput>(schematic, connection_data);

    // pre-conditions
    if (connection.has_connected_element()) [[unlikely]] {
        throw_exception("Connections needs to be unconnected.");
    }

    // find connection at position
    if (const auto found_con = connection_cache.find(position, schematic)) {
        if (found_con->has_connected_element()) {
            if (!found_con->connected_element().is_placeholder()) [[unlikely]] {
                throw_exception("Connection is already connected at this location.");
            }
            // mark placeholder for deletion
            unused_placeholder_id = found_con->connected_element_id();
        }
        // make connection in schematic
        connection.connect(*found_con);
    }

    return unused_placeholder_id;
}

auto EditableCircuit::connect_new_element(element_id_t& element_id) -> void {
    auto delete_queue = delete_queue_t {};
    auto add_if_valid = [&](std::optional<element_id_t> placeholder_id) {
        if (placeholder_id) {
            delete_queue.push_back(*placeholder_id);
        }
    };

    // inputs
    for_each_input_location_and_id(
        schematic_, layout_, element_id,
        [&, element_id](connection_id_t input_id, point_t position) {
            // connect input with output_cache
            const auto placeholder_id = connect_connector_impl(
                {element_id, input_id}, position, output_connections_, schematic_);
            add_if_valid(placeholder_id);
        });

    // outputs
    for_each_output_location_and_id(
        schematic_, layout_, element_id,
        [&, element_id](connection_id_t output_id, point_t position) mutable {
            // connect output with input_cache
            const auto placeholder_id = connect_connector_impl(
                {element_id, output_id}, position, input_connections_, schematic_);
            add_if_valid(placeholder_id);
        });

    cache_insert(element_id);
    add_missing_placeholders(element_id);

    // this invalidates our element_id
    swap_and_delete_multiple_elements(delete_queue);
    element_id = null_element;
}

auto EditableCircuit::cache_insert(element_id_t element_id) -> void {
    input_connections_.insert(element_id, schematic_, layout_);
    output_connections_.insert(element_id, schematic_, layout_);
}

auto EditableCircuit::cache_remove(element_id_t element_id) -> void {
    input_connections_.remove(element_id, schematic_, layout_);
    output_connections_.remove(element_id, schematic_, layout_);
}

auto EditableCircuit::cache_update(element_id_t new_element_id,
                                   element_id_t old_element_id) -> void {
    input_connections_.update(new_element_id, old_element_id, schematic_, layout_);
    output_connections_.update(new_element_id, old_element_id, schematic_, layout_);
}

}  // namespace logicsim
