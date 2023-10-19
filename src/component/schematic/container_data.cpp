#include "component/schematic/container_data.h"

#include "geometry/connection_count.h"

#include <cassert>
#include <exception>

namespace logicsim {

namespace schematic {

auto ContainerData::size() const noexcept -> std::size_t {
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

auto ContainerData::empty() const noexcept -> bool {
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

auto ContainerData::clear() -> void {
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

auto ContainerData::shrink_to_fit() -> void {
    element_types_.shrink_to_fit();
    sub_circuit_ids_.shrink_to_fit();
    input_connections_.shrink_to_fit();
    output_connections_.shrink_to_fit();
    input_inverters_.shrink_to_fit();
    output_delays_.shrink_to_fit();
    history_lengths_.shrink_to_fit();
}

auto ContainerData::swap(ContainerData &other) noexcept -> void {
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

auto ContainerData::add_element(schematic::NewElement &&data) -> element_id_t {
    // check enough space for IDs
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
    // check that sizes match
    if (data.input_inverters.size() != std::size_t {data.input_count}) [[unlikely]] {
        throw std::runtime_error("Need as many values for input_inverters as inputs.");
    }
    if (data.output_delays.size() != std::size_t {data.output_count}) [[unlikely]] {
        throw std::runtime_error("Need as many output_delays as outputs.");
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

auto ContainerData::output(input_t input) const -> output_t {
    return input_connections_.at(input.element_id.value).at(input.element_id.value);
}

auto ContainerData::input(output_t output) const -> input_t {
    return output_connections_.at(output.element_id.value).at(output.element_id.value);
}

auto ContainerData::connect(input_t input, output_t output) -> void {
    clear(input);
    clear(output);

    output_connections_.at(output.element_id.value).at(output.element_id.value) = input;
    input_connections_.at(input.element_id.value).at(input.element_id.value) = output;
}

auto ContainerData::connect(output_t output, input_t input) -> void {
    connect(input, output);
}

auto ContainerData::clear(input_t input) -> void {
    assert(input);

    if (const auto output = this->output(input)) {
        clear_connection(input, output);
    }
}

auto ContainerData::clear(output_t output) -> void {
    if (const auto input = this->input(output)) {
        clear_connection(input, output);
    }
}

auto ContainerData::clear_connection(input_t input, output_t output) -> void {
    input_connections_.at(input.element_id.value).at(input.connection_id.value) =
        null_output;
    output_connections_.at(output.element_id.value).at(output.connection_id.value) =
        null_input;
}

auto ContainerData::clear_all(element_id_t element_id) -> void {
    for (const auto input_id : id_range(input_count(element_id))) {
        clear(input_t {element_id, input_id});
    }

    for (const auto output_id : id_range(output_count(element_id))) {
        clear(output_t {element_id, output_id});
    }
}

auto ContainerData::last_element_id() const -> element_id_t {
    return element_id_t {
        gsl::narrow_cast<element_id_t::value_type>(size() - std::size_t {1}),
    };
}

auto ContainerData::total_input_count() const noexcept -> std::size_t {
    return total_input_count_;
}

auto ContainerData::total_output_count() const noexcept -> std::size_t {
    return total_output_count_;
}

auto ContainerData::input_count(element_id_t element_id) const -> connection_count_t {
    return connection_count_t {input_connections_.at(element_id.value).size()};
}

auto ContainerData::output_count(element_id_t element_id) const -> connection_count_t {
    return connection_count_t {output_connections_.at(element_id.value).size()};
}

auto ContainerData::element_type(element_id_t element_id) const -> ElementType {
    return element_types_.at(element_id.value);
}

auto ContainerData::sub_circuit_id(element_id_t element_id) const -> circuit_id_t {
    return sub_circuit_ids_.at(element_id.value);
}

auto ContainerData::input_inverters(element_id_t element_id) const
    -> const logic_small_vector_t & {
    return input_inverters_.at(element_id.value);
}

auto ContainerData::output_delays(element_id_t element_id) const
    -> const output_delays_t & {
    return output_delays_.at(element_id.value);
}

auto ContainerData::history_length(element_id_t element_id) const -> delay_t {
    return history_lengths_.at(element_id.value);
}

//
// Free Functions
//

auto swap(ContainerData &a, ContainerData &b) noexcept -> void {
    a.swap(b);
}

auto has_input_connections(const ContainerData &data, element_id_t element_id) -> bool {
    assert(element_id);

    const auto is_input_connected = [&](connection_id_t input_id) {
        return bool {data.output(input_t {element_id, input_id})};
    };
    return std::ranges::any_of(id_range(data.input_count(element_id)),
                               is_input_connected);
}

auto has_output_connections(const ContainerData &data, element_id_t element_id) -> bool {
    assert(element_id);

    const auto is_output_connected = [&](connection_id_t output_id) {
        return bool {data.input(output_t {element_id, output_id})};
    };
    return std::ranges::any_of(id_range(data.output_count(element_id)),
                               is_output_connected);
}

}  // namespace schematic

}  // namespace logicsim

template <>
auto std::swap(logicsim::schematic::ContainerData &a,
               logicsim::schematic::ContainerData &b) noexcept -> void {
    a.swap(b);
}