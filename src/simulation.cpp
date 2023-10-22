
#include "simulation.h"

#include "algorithm/range.h"
#include "algorithm/transform_to_container.h"
#include "component/simulation/history_entry.h"
#include "component/simulation/history_view.h"
#include "component/simulation/simulation_event.h"
#include "component/simulation/simulation_event_group.h"
#include "exception.h"
#include "layout_info.h"
#include "logging.h"
#include "logic_item/simulation_info.h"

#include <gsl/gsl>

#include <algorithm>
#include <cmath>

namespace logicsim {

auto set_default_outputs(Simulation &simulation) -> void {
    if (simulation.is_initialized()) [[unlikely]] {
        throw_exception("cannot set outputs for initialized simulation");
    }

    for (const auto element : simulation.schematic().elements()) {
        if (element.is_logic_item()) {
            for (auto output : element.outputs()) {
                if (output.has_connected_element()) {
                    simulation.set_output_value(output, false);
                }
            }
        }
    }
}

auto set_default_inputs(Simulation &simulation) -> void {
    if (simulation.is_initialized()) [[unlikely]] {
        throw_exception("cannot set inputs for initialized simulation");
    }

    for (const auto element : simulation.schematic().elements()) {
        // activate unconnected enable
        if (const auto enable_id = element_enable_input_id(element.element_type())) {
            const auto input = element.input(*enable_id);
            if (!input.has_connected_element() && !input.is_inverted()) {
                simulation.set_input_value(input, true);
            }
        }

        // activate unconnected j&k flipflops
        if (element.element_type() == ElementType::flipflop_jk) {
            const auto input_1 = element.input(connection_id_t {1});
            const auto input_2 = element.input(connection_id_t {2});
            if (!input_1.has_connected_element() && !input_1.is_inverted() &&
                !input_2.has_connected_element() && !input_2.is_inverted()) {
                simulation.set_input_value(input_1, true);
                simulation.set_input_value(input_2, true);
            }
        }
    }
}

//
// Simulation
//

Simulation::Simulation(const SchematicOld &schematic, const PrintEvents print_events)
    : schematic_ {&schematic},
      queue_ {},
      largest_history_event_ {queue_.time()},
      is_initialized_ {false},
      print_events_ {print_events == PrintEvents::yes} {
    input_values_.reserve(schematic.element_count());
    internal_states_.reserve(schematic.element_count());

    for (auto element : schematic.elements()) {
        input_values_.emplace_back(element.input_count().count(), false);
        internal_states_.emplace_back(internal_state_size(element.element_type()), false);
    }
    first_input_histories_.resize(schematic.element_count());
}

auto Simulation::schematic() const noexcept -> const SchematicOld & {
    return *schematic_;
}

auto Simulation::time() const noexcept -> time_t {
    return queue_.time();
}

auto Simulation::submit_event(SchematicOld::ConstInput input, delay_t offset, bool value)
    -> void {
    queue_.submit_event(
        simulation::make_event(input, time_t {queue_.time() + offset}, value));
}

auto Simulation::submit_events(SchematicOld::ConstElement element, delay_t offset,
                               logic_small_vector_t values) -> void {
    if (connection_count_t {std::size(values)} != element.input_count()) [[unlikely]] {
        throw_exception("Need to provide number of input values.");
    }
    for (auto input : element.inputs()) {
        const auto value = values.at(input.input_index().value);
        submit_event(input, offset, value);
    }
}

auto Simulation::check_counts_valid() const -> void {
    const auto n_elements = schematic_->element_count();

    if (input_values_.size() != n_elements || internal_states_.size() != n_elements ||
        first_input_histories_.size() != n_elements) [[unlikely]] {
        throw_exception("size of vector match schematic element count.");
    }
}

auto Simulation::apply_events(const SchematicOld::ConstElement element,
                              const simulation::EventGroup &group) -> void {
    for (const auto &event : group) {
        set_input_internal(element.input(event.input_id), event.value);
    }
}

namespace {
using policy = folly::small_vector_policy::policy_size_type<uint32_t>;

using con_index_small_vector_t = folly::small_vector<connection_id_t, 10, policy>;

static_assert(sizeof(con_index_small_vector_t) == 24);

static_assert(con_index_small_vector_t::max_size() >=
              std::size_t {connection_count_t::max()});
}  // namespace

auto get_changed_outputs(const logic_small_vector_t &old_outputs,
                         const logic_small_vector_t &new_outputs)
    -> con_index_small_vector_t {
    if (std::size(old_outputs) != std::size(new_outputs)) [[unlikely]] {
        throw_exception("old_outputs and new_outputs need to have the same size.");
    }

    auto result = con_index_small_vector_t {};
    for (auto index :
         range(gsl::narrow<connection_id_t::value_type>(std::size(old_outputs)))) {
        if (old_outputs[index] != new_outputs[index]) {
            result.push_back(connection_id_t {index});
        }
    }
    return result;
}

void Simulation::create_event(const SchematicOld::ConstOutput output,
                              const logic_small_vector_t &output_values) {
    if (output.has_connected_element()) {
        queue_.submit_event({.time = queue_.time() + output.delay(),
                             .element_id = output.connected_element_id(),
                             .input_id = output.connected_input_index(),
                             .value = output_values.at(output.output_index().value)});
    }
}

auto Simulation::submit_events_for_changed_outputs(
    const SchematicOld::ConstElement element, const logic_small_vector_t &old_outputs,
    const logic_small_vector_t &new_outputs) -> void {
    const auto changes = get_changed_outputs(old_outputs, new_outputs);
    for (auto output_index : changes) {
        create_event(element.output(output_index), new_outputs);
    }
}

auto invert_inputs(logic_small_vector_t &values, const logic_small_vector_t &inverters)
    -> void {
    if (std::size(values) != std::size(inverters)) [[unlikely]] {
        throw_exception("Inputs and inverters need to have same size.");
    }
    for (auto i : range(std::ssize(values))) {
        values[i] ^= inverters[i];
    }
}

auto inverted_inputs(logic_small_vector_t values, const logic_small_vector_t &inverters)
    -> logic_small_vector_t {
    invert_inputs(values, inverters);
    return values;
}

auto Simulation::process_event_group(simulation::EventGroup &&events) -> void {
    if (print_events_) {
        print_fmt("events: {:n}\n", events);
    }
    if (events.empty()) {
        return;
    }
    const auto element =
        SchematicOld::ConstElement {schematic_->element(events.front().element_id)};
    const auto element_type = element.element_type();

    // short-circuit trivial elements
    if (has_no_logic(element_type)) {
        apply_events(element, events);
        return;
    }

    // update inputs
    auto old_inputs = input_values(element);
    apply_events(element, events);
    auto new_inputs = input_values(element);

    const auto inverters = element.input_inverters();
    if (std::ranges::any_of(inverters, std::identity {})) {
        invert_inputs(old_inputs, inverters);
        invert_inputs(new_inputs, inverters);
    }

    if (has_internal_state(element_type)) {
        auto &internal_state = internal_states_.at(element.element_id().value);

        const auto old_outputs = calculate_outputs_from_state(
            internal_state, element.output_count(), element_type);
        update_internal_state(old_inputs, new_inputs, element_type, internal_state);
        const auto new_outputs = calculate_outputs_from_state(
            internal_state, element.output_count(), element_type);

        submit_events_for_changed_outputs(element, old_outputs, new_outputs);
    } else {
        // find changing outputs
        const auto old_outputs = calculate_outputs_from_inputs(
            old_inputs, element.output_count(), element_type);
        const auto new_outputs = calculate_outputs_from_inputs(
            new_inputs, element.output_count(), element_type);

        submit_events_for_changed_outputs(element, old_outputs, new_outputs);
    }
}

auto Simulation::run(const delay_t simulation_time, const simulation::timeout_t timeout,
                     const int64_t max_events) -> int64_t {
    if (!is_initialized_) {
        throw_exception("Simulation first needs to be initialized.");
    }
    if (simulation_time < delay_t {0us}) [[unlikely]] {
        throw_exception("simulation_time needs to be positive.");
    }
    if (max_events < 0) [[unlikely]] {
        throw_exception("max events needs to be positive or zero.");
    }
    check_counts_valid();  // TODO remove after using value types

    if (simulation_time == delay_t {0us}) {
        return 0;
    }

    const auto timer = TimeoutTimer {timeout};
    const auto queue_end_time =
        simulation_time == simulation::defaults::infinite_simulation_time
            ? time_t::max()
            : queue_.time() + simulation_time;
    int64_t event_count = 0;

    // only check time after this many events
    constexpr int64_t check_interval = 1'000;
    int64_t next_check = std::min(max_events, timeout == simulation::defaults::no_timeout
                                                  ? std::numeric_limits<int64_t>::max()
                                                  : check_interval);

    while (!queue_.empty() && queue_.next_event_time() < queue_end_time) {
        auto event_group = queue_.pop_event_group();
        event_count += std::ssize(event_group);

        process_event_group(std::move(event_group));

        if (event_count >= next_check) {
            // process all events at this time-point
            if (queue_.next_event_time() == queue_.time()) {
                continue;
            }

            // we check timeout after we process at least one group
            if (timer.reached_timeout() || event_count >= max_events) {
                return event_count;
            }
            next_check = std::min(max_events, next_check + check_interval);
        }
    }

    if (simulation_time != simulation::defaults::infinite_simulation_time) {
        queue_.set_time(queue_end_time);
    }
    return event_count;
}

auto Simulation::run_infinitesimal() -> int64_t {
    return run(delay_t::epsilon());
}

auto Simulation::finished() const -> bool {
    return queue_.empty() && time() >= largest_history_event_;
}

auto Simulation::initialize() -> void {
    if (!queue_.empty()) [[unlikely]] {
        throw_exception("Cannot initialize simulation with scheduled events.");
    }
    check_counts_valid();

    for (auto &&element : schematic_->elements()) {
        auto element_type = element.element_type();

        if (element.is_wire()) {
            continue;
        }

        if (element.output_count() == connection_count_t {0}) {
            continue;
        }

        // existing outputs without inverters
        const auto old_outputs = transform_to_container<logic_small_vector_t>(
            element.outputs(),
            [this](auto output) { return input_value(output.connected_input()); });

        if (has_internal_state(element_type)) {
            const auto new_inputs = input_values(element);
            if (std::ranges::any_of(new_inputs, std::identity {})) {
                const auto old_inputs = logic_small_vector_t(new_inputs.size(), false);
                auto &internal_state = internal_states_.at(element.element_id().value);
                update_internal_state(old_inputs, new_inputs, element_type,
                                      internal_state);
            }

            const auto new_outputs = calculate_outputs_from_state(
                internal_state(element), element.output_count(), element_type);
            submit_events_for_changed_outputs(element, old_outputs, new_outputs);

        } else {
            auto curr_inputs = input_values(element);
            invert_inputs(curr_inputs, element.input_inverters());
            const auto new_outputs = calculate_outputs_from_inputs(
                curr_inputs, element.output_count(), element_type);

            submit_events_for_changed_outputs(element, old_outputs, new_outputs);
        }
    }

    is_initialized_ = true;
}

auto Simulation::is_initialized() const -> bool {
    return is_initialized_;
}

auto Simulation::record_input_history(const SchematicOld::ConstInput input,
                                      const bool new_value) -> void {
    // we only record the first input, as we only need a history for wires
    if (input.input_index() != connection_id_t {0}) {
        return;
    }
    const auto history_length = input.element().history_length();

    if (history_length <= delay_t {0ns}) {
        return;
    }
    if (new_value == input_value(input)) {
        return;
    }
    auto &history = first_input_histories_.at(input.element_id().value);

    // remove old values
    clean_history(history, history_length);

    // add new entry
    history.push_back(time());

    // update largest history event
    largest_history_event_ = std::max(largest_history_event_, time() + history_length);
}

auto Simulation::clean_history(simulation::HistoryBuffer &history, delay_t history_length)
    -> void {
    const auto min_time = time() - history_length;
    while (!history.empty() && history.front() < min_time) {
        history.pop_front();
    }
}

auto Simulation::input_value(element_id_t element_id, connection_id_t index) const
    -> bool {
    return input_values_.at(element_id.value).at(index.value);
}

auto Simulation::input_value(const SchematicOld::ConstInput input) const -> bool {
    return input_value(input.element_id(), input.input_index());
}

auto Simulation::input_values(element_id_t element_id) const
    -> const logic_small_vector_t & {
    return input_values_.at(element_id.value);
}

auto Simulation::input_values(SchematicOld::Element element) const
    -> const logic_small_vector_t & {
    return input_values(element.element_id());
}

auto Simulation::input_values(const SchematicOld::ConstElement element) const
    -> const logic_small_vector_t & {
    return input_values(element.element_id());
}

auto Simulation::set_input_internal(const SchematicOld::ConstInput input, bool value)
    -> void {
    record_input_history(input, value);

    input_values_.at(input.element_id().value).at(input.input_index().value) = value;
}

auto Simulation::set_output_value(SchematicOld::ConstOutput output, bool value) -> void {
    if (is_initialized_) {
        throw_exception("can only set outputs at the start of the simulation");
    }

    const auto input = output.connected_input();

    auto &input_value =
        input_values_.at(input.element_id().value).at(input.input_index().value);
    input_value = value ^ input.is_inverted();
}

auto Simulation::output_value(element_id_t element_id, connection_id_t index) const
    -> bool {
    return output_value(schematic_->element(element_id).output(index));
}

auto Simulation::output_value(const SchematicOld::ConstOutput output) const -> bool {
    const auto input = output.connected_input();
    return input_value(input) ^ input.is_inverted();
}

auto Simulation::output_values(element_id_t element_id) const -> logic_small_vector_t {
    return output_values(schematic_->element(element_id));
}

auto Simulation::output_values(SchematicOld::Element element) const
    -> logic_small_vector_t {
    return output_values(SchematicOld::ConstElement {element});
}

auto Simulation::output_values(const SchematicOld::ConstElement element) const
    -> logic_small_vector_t {
    return transform_to_container<logic_small_vector_t>(
        element.outputs(), [=, this](auto output) { return output_value(output); });
}

auto Simulation::set_input_value(SchematicOld::ConstInput input, bool value) -> void {
    if (input.has_connected_element()) [[unlikely]] {
        throw_exception("cannot input-value for connected inputs");
    }
    auto &input_value =
        input_values_.at(input.element_id().value).at(input.input_index().value);

    if (!is_initialized_) {
        input_value = value;
    } else {
        if (input_value != value) {
            submit_event(input, delay_t::epsilon(), value);
            run_infinitesimal();
        }
    }
}

auto Simulation::set_internal_state(SchematicOld::ConstElement element, std::size_t index,
                                    bool value) -> void {
    auto &state = internal_states_.at(element.element_id().value).at(index);

    if (!is_initialized_) {
        state = value;
    } else {
        const auto old_outputs = calculate_outputs_from_state(
            internal_state(element), element.output_count(), element.element_type());
        state = value;
        const auto new_outputs = calculate_outputs_from_state(
            internal_state(element), element.output_count(), element.element_type());

        submit_events_for_changed_outputs(element, old_outputs, new_outputs);
        run_infinitesimal();
    }
}

auto Simulation::internal_state(element_id_t element_id) const
    -> const logic_small_vector_t & {
    return internal_states_.at(element_id.value);
}

auto Simulation::internal_state(SchematicOld::Element element) const
    -> const logic_small_vector_t & {
    return internal_state(element.element_id());
}

auto Simulation::internal_state(SchematicOld::ConstElement element) const
    -> const logic_small_vector_t & {
    return internal_state(element.element_id());
}

auto Simulation::internal_state(SchematicOld::ConstElement element,
                                std::size_t index) const -> bool {
    return internal_state(element).at(index);
}

auto Simulation::input_history(element_id_t element_id) const -> simulation::HistoryView {
    return input_history(schematic_->element(element_id));
}

auto Simulation::input_history(SchematicOld::Element element) const
    -> simulation::HistoryView {
    return input_history(SchematicOld::ConstElement {element});
}

auto Simulation::input_history(SchematicOld::ConstElement element) const
    -> simulation::HistoryView {
    const auto &input_values = this->input_values(element);

    if (input_values.empty()) {
        return simulation::HistoryView {};
    }

    const auto last_value =
        static_cast<bool>(input_values.at(0) ^ element.input_inverters().at(0));

    return simulation::HistoryView {
        first_input_histories_.at(element.element_id().value),
        this->time(),
        last_value,
        element.history_length(),
    };
}

}  // namespace logicsim
