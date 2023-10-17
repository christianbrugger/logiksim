#include "component/schematic/container_data.h"

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

auto ContainerData::swap_element_data(element_id_t element_id_1,
                                      element_id_t element_id_2, bool update_connections)
    -> void {
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

    if (update_connections) {
        update_swapped_connections(element_id_1, element_id_2);
        update_swapped_connections(element_id_2, element_id_1);
    }
}

auto ContainerData::delete_last_element(bool clear_connections) -> void {
    if (empty()) [[unlikely]] {
        throw std::runtime_error("Cannot delete from empty schematics.");
    }

    // TODO implement
    throw std::runtime_error("implement");
    /*
    if (clear_connections) {
        const auto last_id = last_element_id();
        element(last_id).clear_all_connection();
    }
    */

    // decrease counts
    const auto last_input_count = input_connections_.back().size();
    const auto last_output_count = output_connections_.back().size();
    if (total_input_count_ < last_input_count || total_output_count_ < last_output_count)
        [[unlikely]] {
        throw std::runtime_error("input or output count underflows");
    }
    total_input_count_ -= last_input_count;
    total_output_count_ -= last_output_count;

    // delete element
    element_types_.pop_back();
    sub_circuit_ids_.pop_back();
    input_connections_.pop_back();
    output_connections_.pop_back();
    input_inverters_.pop_back();
    output_delays_.pop_back();
    history_lengths_.pop_back();
}

auto ContainerData::swap_and_delete_element(element_id_t element_id) -> element_id_t {
    const auto last_id = last_element_id();

    // TODO implement
    throw std::runtime_error("implement");
    /*
    element(element_id).clear_all_connection();
    */

    if (element_id != last_id) {
        swap_element_data(element_id, last_id, false);
        update_swapped_connections(element_id, last_id);
    }

    delete_last_element(false);
    return last_id;
}

auto ContainerData::swap_elements(element_id_t element_id_0, element_id_t element_id_1)
    -> void {
    swap_element_data(element_id_0, element_id_1, true);
}

auto ContainerData::update_swapped_connections(element_id_t new_element_id,
                                               element_id_t old_element_id) -> void {
    if (new_element_id == old_element_id) {
        return;
    }

    // TODO implement
    throw std::runtime_error("implement");
    /*
    const auto old_element = element(old_element_id);
    const auto new_element = element(new_element_id);

    for (auto input : new_element.inputs()) {
        if (input.has_connected_element()) {
            if (input.connected_element_id() == old_element_id) {
                // self connection?
                input.connect(new_element.output(input.connected_output_index()));
            } else if (input.connected_element_id() == new_element_id) {
                // swapped connection?
                input.connect(old_element.output(input.connected_output_index()));
            } else {
                // fix back connection
                input.connect(input.connected_output());
            }
        }
    }

    for (auto output : new_element.outputs()) {
        if (output.has_connected_element()) {
            if (output.connected_element_id() == old_element_id) {
                // self connection?
                output.connect(new_element.input(output.connected_input_index()));
            } else if (output.connected_element_id() == new_element_id) {
                // swapped connection?
                output.connect(old_element.input(output.connected_input_index()));
            } else {
                // fix back connection
                output.connect(output.connected_input());
            }
        }
    }
    */
}

auto ContainerData::add_element(schematic::NewElement &&data) -> element_id_t {
    // check ids available
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
    // check sizes match
    if (data.input_inverters.size() != std::size_t {data.input_count}) [[unlikely]] {
        throw std::runtime_error("Need as many values for input_inverters as inputs.");
    }
    if (data.output_delays.size() != std::size_t {data.output_count}) [[unlikely]] {
        throw std::runtime_error("Need as many output_delays as outputs.");
    }

    // add new data
    element_types_.push_back(data.element_type);
    sub_circuit_ids_.push_back(data.sub_circuit_id);
    input_connections_.emplace_back(data.input_count.count(), null_connection);
    output_connections_.emplace_back(data.output_count.count(), null_connection);
    input_inverters_.emplace_back(std::move(data.input_inverters));
    output_delays_.emplace_back(std::move(data.output_delays));
    history_lengths_.push_back(data.history_length);

    // increase counts
    total_input_count_ += std::size_t {data.input_count};
    total_output_count_ += std::size_t {data.output_count};

    return last_element_id();
}

auto ContainerData::last_element_id() const -> element_id_t {
    return element_id_t {
        gsl::narrow_cast<element_id_t::value_type>(size() - std::size_t {1})};
}

auto ContainerData::total_input_count() const noexcept -> std::size_t {
    return total_input_count_;
}

auto ContainerData::total_output_count() const noexcept -> std::size_t {
    return total_output_count_;
}

//
// Free Functions
//

auto swap(ContainerData &a, ContainerData &b) noexcept -> void {
    a.swap(b);
}

}  // namespace schematic

}  // namespace logicsim

template <>
auto std::swap(logicsim::schematic::ContainerData &a,
               logicsim::schematic::ContainerData &b) noexcept -> void {
    a.swap(b);
}