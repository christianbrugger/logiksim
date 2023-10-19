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

/**
 * @brief: swaps element data
 *
 * Warning connection invariant are broken for the swapped ids
 */
auto ContainerData::swap_element_data(element_id_t element_id_1,
                                      element_id_t element_id_2) -> void {
    if (element_id_1 == element_id_2) {
        return;
    }

    const auto swap_ids = [element_id_1, element_id_2](auto &container) {
        using std::swap;
        swap(container.at(element_id_1.value), container.at(element_id_2.value));
    };

    swap_ids(element_types_);
    swap_ids(sub_circuit_ids_);
    swap_ids(input_connections_);
    swap_ids(output_connections_);
    swap_ids(input_inverters_);
    swap_ids(output_delays_);
    swap_ids(history_lengths_);
}

/**
 * @brief: Deletes the last element
 *
 * Throws exception if container is empty.
 *
 * pre-condition:
 *   1) last element has no connections
 */
auto ContainerData::delete_last_unconnected_element() -> void {
    // pre-condition
    assert(!has_input_connections(*this, last_element_id()));
    assert(!has_output_connections(*this, last_element_id()));

    // exceptions
    if (empty()) [[unlikely]] {
        throw std::runtime_error("Cannot delete from empty schematics.");
    }

    // decrease counts
    const auto last_input_count = input_connections_.back().size();
    const auto last_output_count = output_connections_.back().size();
    assert(total_input_count_ >= last_input_count);
    assert(total_output_count_ >= last_output_count);
    total_input_count_ -= last_input_count;
    total_output_count_ -= last_output_count;

    // shrink vectors
    element_types_.pop_back();
    sub_circuit_ids_.pop_back();
    input_connections_.pop_back();
    output_connections_.pop_back();
    input_inverters_.pop_back();
    output_delays_.pop_back();
    history_lengths_.pop_back();
}

auto ContainerData::swap_and_delete_element(element_id_t element_id) -> element_id_t {
    clear_all(element_id);

    const auto last_id = last_element_id();
    if (element_id != last_id) {
        swap_element_data(element_id, last_id);
        update_swapped_connections(element_id, last_id);
    }

    delete_last_unconnected_element();
    return last_id;
}

auto ContainerData::swap_elements(element_id_t element_id_0, element_id_t element_id_1)
    -> void {
    swap_element_data(element_id_0, element_id_1);

    // TODO do we need both ?
    update_swapped_connections(element_id_0, element_id_1);
    update_swapped_connections(element_id_1, element_id_0);
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
    assert(input);
    assert(output);

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

/**
 * brief: Re-writes the connections of two swapped element.
 */
auto ContainerData::update_swapped_connections(element_id_t new_element_id,
                                               element_id_t old_element_id) -> void {
    if (new_element_id == old_element_id) {
        return;
    }

    throw std::runtime_error("implement");
    /*
    const auto transform_id = [&](element_id_t connection_element_id) {
        // self connection
        if (connection_element_id == old_element_id) {
            return new_element_id;
        }
        // swapped connection
        if (connection_element_id == new_element_id) {
            return old_element_id;
        }
        return connection_element_id;
    };

    for (const auto input_id : id_range(input_count(new_element_id))) {
        const auto new_input = input_t {new_element_id, input_id};

        if (const auto old_output = connection(new_input)) {
            const auto new_output = output_t {
                transform_id(old_output.element_id),
                old_output.connection_id,
            };
            // TODO only use connect when transformed != old & new
            // TODO otherwise directly write only this side of the connection
            connect(new_input, new_output);
        }
    }

    for (const auto output_id : id_range(output_count(new_element_id))) {
        const auto new_output = output_t {new_element_id, output_id};

        if (const auto old_input = connection(new_output)) {
            const auto new_input = input_t {
                transform_id(old_input.element_id),
                old_input.connection_id,
            };
            connect(new_input, new_output);
        }
    }
    */
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
    const auto is_input_connected = [&](connection_id_t input_id) {
        return bool {data.output(input_t {element_id, input_id})};
    };
    return std::ranges::any_of(id_range(data.input_count(element_id)),
                               is_input_connected);
}

auto has_output_connections(const ContainerData &data, element_id_t element_id) -> bool {
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