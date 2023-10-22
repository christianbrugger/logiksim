#include "schematic_validation.h"

#include "algorithm/accumulate.h"
#include "iterator_adaptor/transform_view.h"
#include "layout_info.h"
#include "schematic_old.h"
#include "schematic_validation.h"

#include <stdexcept>

namespace logicsim {

auto validate_input_connected(const SchematicOld::ConstInput input) -> void {
    if (!input.has_connected_element()) [[unlikely]] {
        throw std::runtime_error("Element has unconnected input.");
    }
}

auto validate_output_connected(const SchematicOld::ConstOutput output) -> void {
    if (!output.has_connected_element()) [[unlikely]] {
        throw std::runtime_error("Element has unconnected output.");
    }
}

auto validate_input_disconnected(const SchematicOld::ConstInput input) -> void {
    if (input.has_connected_element()) [[unlikely]] {
        throw std::runtime_error("Element has connected input.");
    }
}

auto validate_output_disconnected(const SchematicOld::ConstOutput output) -> void {
    if (output.has_connected_element()) [[unlikely]] {
        throw std::runtime_error("Element has connected output.");
    }
}

auto validate_all_outputs_connected(const SchematicOld::ConstElement element) -> void {
    std::ranges::for_each(element.outputs(), validate_output_connected);
}

auto validate_all_non_wire_outputs_connected(const SchematicOld::ConstElement element)
    -> void {
    for (const auto output : element.outputs()) {
        if (!element.is_wire()) {
            validate_output_connected(output);
        }
    }
}

auto validate_all_inputs_disconnected(const SchematicOld::ConstElement element) -> void {
    std::ranges::for_each(element.inputs(), validate_input_disconnected);
}

auto validate_all_outputs_disconnected(const SchematicOld::ConstElement element) -> void {
    std::ranges::for_each(element.outputs(), validate_output_disconnected);
}

auto validate_placeholder_connected(const SchematicOld::ConstElement element) -> void {
    if (element.element_type() == ElementType::placeholder) {
        std::ranges::for_each(element.inputs(), validate_input_connected);
        std::ranges::for_each(element.outputs(), validate_output_connected);
    }
}

auto validate_has_no_placeholders(const SchematicOld::ConstElement element) -> void {
    const auto is_placeholder = [](const SchematicOld::ConstOutput output) {
        return output.has_connected_element() &&
               output.connected_element().is_placeholder();
    };
    if (std::ranges::any_of(element.outputs(), is_placeholder)) {
        throw std::runtime_error("element should not have output placeholders");
    }
}

auto validate_input_has_connection_valid(const SchematicOld::ConstInput input) -> void {
    if (input.has_connected_element() != bool {input.connected_element_id()} ||
        input.has_connected_element() != bool {input.connected_output_index()}) {
        throw std::runtime_error("has_connected_element is inconsistent");
    }
}

auto validate_output_has_connection_valid(const SchematicOld::ConstOutput output)
    -> void {
    if (output.has_connected_element() != bool {output.connected_element_id()} ||
        output.has_connected_element() != bool {output.connected_input_index()}) {
        throw std::runtime_error("has_connected_element is inconsistent");
    }
}

auto validate_has_connection_valid(const SchematicOld::ConstElement element) -> void {
    std::ranges::for_each(element.inputs(), validate_input_has_connection_valid);
    std::ranges::for_each(element.outputs(), validate_output_has_connection_valid);
}

auto validate_input_consistent(const SchematicOld::ConstInput input) -> void {
    if (input.has_connected_element()) {
        if (!input.connected_output().has_connected_element()) [[unlikely]] {
            throw std::runtime_error("Back reference is missing.");
        }
        auto back_reference {input.connected_output().connected_input()};
        if (back_reference != input) [[unlikely]] {
            throw std::runtime_error("Back reference doesn't match.");
        }
    }
}

auto validate_output_consistent(const SchematicOld::ConstOutput output) -> void {
    if (output.has_connected_element()) {
        if (!output.connected_input().has_connected_element()) [[unlikely]] {
            throw std::runtime_error("Back reference is missing.");
        }
        auto back_reference {output.connected_input().connected_output()};
        if (back_reference != output) [[unlikely]] {
            throw std::runtime_error("Back reference doesn't match.");
        }
    }
}

auto validate_element_connections_consistent(const SchematicOld::ConstElement element)
    -> void {
    std::ranges::for_each(element.inputs(), validate_input_consistent);
    std::ranges::for_each(element.outputs(), validate_output_consistent);
}

auto validate_no_input_loops(const SchematicOld::ConstInput input) -> void {
    // clocks have an internal loop, that's allowed.
    const auto clock_loop = [=]() {
        return input.element().element_type() == ElementType::clock_generator &&
               ((input.input_index() == connection_id_t {1} &&
                 input.connected_output_index() == connection_id_t {1}) ||
                (input.input_index() == connection_id_t {2} &&
                 input.connected_output_index() == connection_id_t {2}));
    };

    if (input.connected_element_id() == input.element_id() && !clock_loop())
        [[unlikely]] {
        throw std::runtime_error("element connects to itself, loops are not allowed.");
    }
}

auto validate_no_output_loops(const SchematicOld::ConstOutput output) -> void {
    // clocks have an internal loop, that's allowed.
    const auto clock_loop = [=]() {
        return output.element().element_type() == ElementType::clock_generator &&
               ((output.output_index() == connection_id_t {1} &&
                 output.connected_input_index() == connection_id_t {1}) ||
                (output.output_index() == connection_id_t {2} &&
                 output.connected_input_index() == connection_id_t {2}));
    };

    if (output.connected_element_id() == output.element_id() && !clock_loop())
        [[unlikely]] {
        throw std::runtime_error("element connects to itself, loops are not allowed.");
    }
}

auto validate_element_connections_no_loops(const SchematicOld::ConstElement element)
    -> void {
    std::ranges::for_each(element.inputs(), validate_no_input_loops);
    std::ranges::for_each(element.outputs(), validate_no_output_loops);
}

auto validate_input_output_count(const SchematicOld::ConstElement element) -> void {
    if (!is_input_output_count_valid(element.element_type(), element.input_count(),
                                     element.output_count())) [[unlikely]] {
        throw std::runtime_error("element has wrong input or output count.");
    }
}

auto validate_connection_data(const connection_t connection_data) -> void {
    if (connection_data.element_id != null_element &&
        connection_data.connection_id == null_connection_id) [[unlikely]] {
        throw std::runtime_error(
            "Connection to an element cannot have null_connection_id.");
    }

    if (connection_data.element_id == null_element &&
        connection_data.connection_id != null_connection_id) [[unlikely]] {
        throw std::runtime_error(
            "Connection with null_element requires null_connection_id.");
    }
}

auto validate_sub_circuit_ids(const SchematicOld::ConstElement element) -> void {
    if (!(element.is_sub_circuit() == bool {element.sub_circuit_id()})) [[unlikely]] {
        throw std::runtime_error("Not a sub-circuit or no circuit id.");
    }
}

auto validate(const SchematicOld &schematic, schematic::ValidationSettings settings)
    -> void {
    // connections
    std::ranges::for_each(schematic.elements(), validate_input_output_count);
    std::ranges::for_each(schematic.elements(), validate_has_connection_valid);
    std::ranges::for_each(schematic.elements(), validate_element_connections_consistent);
    std::ranges::for_each(schematic.elements(), validate_element_connections_no_loops);

    if (settings.require_all_outputs_connected) {
        std::ranges::for_each(schematic.elements(),
                              validate_all_non_wire_outputs_connected);
    }

    if (settings.require_all_placeholders_connected) {
        std::ranges::for_each(schematic.elements(), validate_placeholder_connected);
    }

    // simulation attributes
    std::ranges::for_each(schematic.elements(), validate_sub_circuit_ids);

    // TODO check new data members
    // * input_inverters_
    // * output_delays_
    // * history_lengths_

    // global attributes
    if (!schematic.circuit_id()) [[unlikely]] {
        throw std::runtime_error("invalid circuit id");
    }

    const auto to_input_size =
        [](const SchematicOld::ConstElement &element) -> std::size_t {
        return std::size_t {element.input_count()};
    };
    const auto to_output_size =
        [](const SchematicOld::ConstElement &element) -> std::size_t {
        return std::size_t {element.output_count()};
    };

    const auto input_count =
        accumulate(transform_view(schematic.elements(), to_input_size), std::size_t {0});
    const auto output_count =
        accumulate(transform_view(schematic.elements(), to_output_size), std::size_t {0});

    if (input_count != schematic.total_input_count()) [[unlikely]] {
        throw std::runtime_error("input count is wrong");
    }
    if (output_count != schematic.total_output_count()) [[unlikely]] {
        throw std::runtime_error("input count is wrong");
    }
}

void validate(const Schematic &schematic, schematic::ValidationSettings settings) {
    return;
}

}  // namespace logicsim
