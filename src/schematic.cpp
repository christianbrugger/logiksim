#include "schematic.h"

#include "algorithm/fmt_join.h"
#include "geometry/connection_count.h"
#include "iterator_adaptor/transform_view.h"
#include "logic_item/schematic_info.h"

#include <fmt/core.h>

#include <algorithm>
#include <cassert>
#include <exception>
#include <ranges>

namespace logicsim {

auto Schematic::size() const noexcept -> std::size_t {
    const auto size = element_types_.size();

    assert(size == element_types_.size());
    assert(size == sub_circuit_ids_.size());
    assert(size == input_connections_.size());
    assert(size == output_connections_.size());
    assert(size == input_inverters_.size());
    assert(size == output_delays_.size());
    assert(size == history_lengths_.size());

    return size;
}

auto Schematic::empty() const noexcept -> bool {
    const auto empty = element_types_.empty();

    assert(empty == element_types_.empty());
    assert(empty == sub_circuit_ids_.empty());
    assert(empty == input_connections_.empty());
    assert(empty == output_connections_.empty());
    assert(empty == input_inverters_.empty());
    assert(empty == output_delays_.empty());
    assert(empty == history_lengths_.empty());

    return empty;
}

auto Schematic::clear() -> void {
    element_types_.clear();
    sub_circuit_ids_.clear();
    input_connections_.clear();
    output_connections_.clear();
    input_inverters_.clear();
    output_delays_.clear();
    history_lengths_.clear();

    total_input_count_ = 0;
    total_output_count_ = 0;
}

auto Schematic::reserve(std::size_t new_capacity) -> void {
    element_types_.reserve(new_capacity);
    sub_circuit_ids_.reserve(new_capacity);
    input_connections_.reserve(new_capacity);
    output_connections_.reserve(new_capacity);
    input_inverters_.reserve(new_capacity);
    output_delays_.reserve(new_capacity);
    history_lengths_.reserve(new_capacity);
}

auto Schematic::shrink_to_fit() -> void {
    element_types_.shrink_to_fit();
    sub_circuit_ids_.shrink_to_fit();
    input_connections_.shrink_to_fit();
    output_connections_.shrink_to_fit();
    input_inverters_.shrink_to_fit();
    output_delays_.shrink_to_fit();
    history_lengths_.shrink_to_fit();
}

auto Schematic::swap(Schematic &other) noexcept -> void {
    using std::swap;

    element_types_.swap(other.element_types_);
    sub_circuit_ids_.swap(other.sub_circuit_ids_);
    input_connections_.swap(other.input_connections_);
    output_connections_.swap(other.output_connections_);
    input_inverters_.swap(other.input_inverters_);
    output_delays_.swap(other.output_delays_);
    history_lengths_.swap(other.history_lengths_);

    swap(total_input_count_, other.total_input_count_);
    swap(total_output_count_, other.total_output_count_);
}

auto Schematic::format() const -> std::string {
    if (empty()) {
        return fmt::format("<Schematic with {} elements>", size());
    }

    const auto format_element = [&](element_id_t element_id) -> std::string {
        return format_element_with_connections(*this, element_id);
    };
    const auto list = fmt_join(",\n  ", element_ids(*this), "{}", format_element);
    return fmt::format("<Schematic with {} elements: [\n  {}\n]>", size(), list);
}

auto Schematic::add_element(schematic::NewElement &&data) -> element_id_t {
    // check enough space for IDs
    static_assert(decltype(element_types_) {}.max_size() >=
                  std::size_t {element_id_t::max()});
    if (element_types_.size() >= std::size_t {element_id_t::max()}) [[unlikely]] {
        throw std::runtime_error("Reached maximum number of elements.");
    }
    if (total_input_count_ >= std::numeric_limits<std::size_t>::max() -
                                  std::size_t {data.input_count.count()}) [[unlikely]] {
        throw std::runtime_error("Reached maximum number of inputs.");
    }
    if (total_output_count_ >= std::numeric_limits<std::size_t>::max() -
                                   std::size_t {data.output_count.count()}) [[unlikely]] {
        throw std::runtime_error("Reached maximum number of outputs.");
    }
    // check input sizes match
    if (data.input_inverters.size() != std::size_t {data.input_count}) [[unlikely]] {
        throw std::runtime_error("Need as many values for input_inverters as inputs.");
    }
    if (data.output_delays.size() != std::size_t {data.output_count}) [[unlikely]] {
        throw std::runtime_error("Need as many output_delays as outputs.");
    }
    // check delay_t positive
    const auto delay_positive = [](delay_t delay) { return delay > delay_t::zero(); };
    if (!std::ranges::all_of(data.output_delays, delay_positive)) [[unlikely]] {
        throw std::runtime_error("delays need to be positive");
    }
    if (data.history_length < delay_t::zero()) [[unlikely]] {
        throw std::runtime_error("history length cannot be negative");
    }
    // check input & output count
    if (!is_input_output_count_valid(data.element_type, data.input_count,
                                     data.output_count)) [[unlikely]] {
        throw std::runtime_error("input or output count not valid");
    }

    // add new data
    element_types_.push_back(data.element_type);
    sub_circuit_ids_.push_back(data.sub_circuit_id);
    input_connections_.emplace_back(data.input_count.count(), null_output);
    output_connections_.emplace_back(data.output_count.count(), null_input);
    input_inverters_.emplace_back(std::move(data.input_inverters));
    output_delays_.emplace_back(std::move(data.output_delays));
    history_lengths_.push_back(data.history_length);

    // increase counts
    total_input_count_ += std::size_t {data.input_count};
    total_output_count_ += std::size_t {data.output_count};

    return last_element_id();
}

auto Schematic::output(input_t input) const -> output_t {
    return input_connections_.at(input.element_id.value).at(input.connection_id.value);
}

auto Schematic::input(output_t output) const -> input_t {
    return output_connections_.at(output.element_id.value).at(output.connection_id.value);
}

auto Schematic::connect(input_t input, output_t output) -> void {
    clear(input);
    clear(output);

    output_connections_.at(output.element_id.value).at(output.connection_id.value) =
        input;
    input_connections_.at(input.element_id.value).at(input.connection_id.value) = output;
}

auto Schematic::connect(output_t output, input_t input) -> void {
    connect(input, output);
}

auto Schematic::clear(input_t input) -> void {
    if (const auto output = this->output(input)) {
        clear_connection(input, output);
    }
}

auto Schematic::clear(output_t output) -> void {
    if (const auto input = this->input(output)) {
        clear_connection(input, output);
    }
}

auto Schematic::clear_connection(input_t input, output_t output) -> void {
    input_connections_.at(input.element_id.value).at(input.connection_id.value) =
        null_output;
    output_connections_.at(output.element_id.value).at(output.connection_id.value) =
        null_input;
}

auto Schematic::clear_all_connections(element_id_t element_id) -> void {
    for (const auto input_id : id_range(input_count(element_id))) {
        clear(input_t {element_id, input_id});
    }

    for (const auto output_id : id_range(output_count(element_id))) {
        clear(output_t {element_id, output_id});
    }
}

auto Schematic::last_element_id() const -> element_id_t {
    return element_id_t {
        gsl::narrow_cast<element_id_t::value_type>(size() - std::size_t {1}),
    };
}

auto Schematic::total_input_count() const noexcept -> std::size_t {
    return total_input_count_;
}

auto Schematic::total_output_count() const noexcept -> std::size_t {
    return total_output_count_;
}

auto Schematic::input_count(element_id_t element_id) const -> connection_count_t {
    return connection_count_t {input_connections_.at(element_id.value).size()};
}

auto Schematic::output_count(element_id_t element_id) const -> connection_count_t {
    return connection_count_t {output_connections_.at(element_id.value).size()};
}

auto Schematic::element_type(element_id_t element_id) const -> ElementType {
    return element_types_.at(element_id.value);
}

auto Schematic::sub_circuit_id(element_id_t element_id) const -> circuit_id_t {
    return sub_circuit_ids_.at(element_id.value);
}

auto Schematic::input_inverters(element_id_t element_id) const
    -> const logic_small_vector_t & {
    return input_inverters_.at(element_id.value);
}

auto Schematic::output_delays(element_id_t element_id) const -> const output_delays_t & {
    return output_delays_.at(element_id.value);
}

auto Schematic::history_length(element_id_t element_id) const -> delay_t {
    return history_lengths_.at(element_id.value);
}

auto Schematic::output_delay(output_t output) const -> delay_t {
    return output_delays_.at(output.element_id.value).at(output.connection_id.value);
}

auto Schematic::input_inverted(input_t input) const -> bool {
    return input_inverters_.at(input.element_id.value).at(input.connection_id.value);
}

auto Schematic::set_input_inverter(input_t input, bool value) -> void {
    input_inverters_.at(input.element_id.value).at(input.connection_id.value) = value;
}

//
// Free Functions
//

auto swap(Schematic &a, Schematic &b) noexcept -> void {
    a.swap(b);
}

auto has_input_connections(const Schematic &data, element_id_t element_id) -> bool {
    const auto is_input_connected = [&](connection_id_t input_id) {
        return bool {data.output(input_t {element_id, input_id})};
    };
    return std::ranges::any_of(id_range(data.input_count(element_id)),
                               is_input_connected);
}

auto has_output_connections(const Schematic &data, element_id_t element_id) -> bool {
    const auto is_output_connected = [&](connection_id_t output_id) {
        return bool {data.input(output_t {element_id, output_id})};
    };
    return std::ranges::any_of(id_range(data.output_count(element_id)),
                               is_output_connected);
}

auto element_ids(const Schematic &schematic) -> range_extended_t<element_id_t> {
    return range_extended<element_id_t>(schematic.size());
}

auto input_ids(const Schematic &schematic,
               element_id_t element_id) -> range_extended_t<connection_id_t> {
    return range_extended<connection_id_t>(
        std::size_t {schematic.input_count(element_id)});
}

auto output_ids(const Schematic &schematic,
                element_id_t element_id) -> range_extended_t<connection_id_t> {
    return range_extended<connection_id_t>(
        std::size_t {schematic.output_count(element_id)});
}

auto input_inverted(const Schematic &schematic, input_t input) -> bool {
    return schematic.input_inverters(input.element_id).at(input.connection_id.value);
}

auto format_element(const Schematic &schematic, element_id_t element_id) -> std::string {
    return fmt::format(
        "<Element {}: {}x{} {}>", element_id, schematic.input_count(element_id),
        schematic.output_count(element_id), schematic.element_type(element_id));
}

auto format_element_with_connections(const Schematic &schematic,
                                     element_id_t element_id) -> std::string {
    const auto input_connections =
        transform_view(inputs(schematic, element_id),
                       [&](input_t input) { return schematic.output(input); });
    const auto output_connections =
        transform_view(outputs(schematic, element_id),
                       [&](output_t output) { return schematic.input(output); });

    return fmt::format(
        "<Element {}: {}x{} {}, inputs = {}, outputs = {}>", element_id,
        schematic.input_count(element_id), schematic.output_count(element_id),
        schematic.element_type(element_id), input_connections, output_connections);
}

}  // namespace logicsim

template <>
auto std::swap(logicsim::Schematic &a, logicsim::Schematic &b) noexcept -> void {
    a.swap(b);
}