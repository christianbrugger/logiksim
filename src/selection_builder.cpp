
#include "selection_builder.h"

#include "algorithm.h"
#include "format.h"
#include "range.h"

namespace logicsim {

//
// Selection Builder
//

SelectionBuilder::SelectionBuilder(const EditableCircuit& editable_circuit)
    : editable_circuit_ {editable_circuit} {}

auto SelectionBuilder::clear() -> void {
    initial_selection_.clear();
    operations_.clear();
}

auto SelectionBuilder::add(SelectionFunction function, rect_fine_t rect) -> void {
    operations_.emplace_back(operation_t {function, rect});
}

auto SelectionBuilder::update_last(rect_fine_t rect) -> void {
    if (operations_.empty()) [[unlikely]] {
        throw_exception("Cannot update with empty operations.");
    }
    operations_.back().rect = rect;
}

auto SelectionBuilder::pop_last() -> void {
    if (operations_.empty()) [[unlikely]] {
        throw_exception("Cannot update with empty operations.");
    }
    operations_.pop_back();
}

namespace {

auto apply_function(Selection& selection, const EditableCircuit& editable_circuit,
                    SelectionBuilder::operation_t operation) -> void {
    const auto element_ids = editable_circuit.query_selection(operation.rect);
    const auto element_keys = editable_circuit.to_element_keys(element_ids);

    if (operation.function == SelectionFunction::toggle) {
        for (auto&& element_key : element_keys) {
            selection.toggle_element(element_key);
        }
    }

    if (operation.function == SelectionFunction::add) {
        for (auto&& element_key : element_keys) {
            selection.add_element(element_key);
        }
    }

    if (operation.function == SelectionFunction::substract) {
        for (auto&& element_key : element_keys) {
            selection.remove_element(element_key);
        }
    }
}

}  // namespace

auto SelectionBuilder::calculate_selection() const -> Selection {
    auto selection = Selection {initial_selection_};

    for (auto&& operation : operations_) {
        apply_function(selection, editable_circuit_, operation);
    }

    return selection;
}

// TODO remove
auto SelectionBuilder::create_selection_mask() const -> selection_mask_t {
    if (initial_selection_.empty() && operations_.empty()) {
        return {};
    }

    const auto element_ids = [&]() {
        if (operations_.empty()) {
            return editable_circuit_.to_element_ids(
                initial_selection_.selected_elements());
        }
        // TODO cache selection
        const auto selection = calculate_selection();
        return editable_circuit_.to_element_ids(selection.selected_elements());
    }();

    const auto element_count = editable_circuit_.schematic().element_count();
    auto mask = selection_mask_t(element_count, false);

    for (element_id_t element_id : element_ids) {
        mask.at(element_id.value) = true;
    }
    return mask;
}

auto SelectionBuilder::claculate_is_item_selected(element_key_t element_key) const
    -> bool {
    if (element_key < element_key_t {0}) [[unlikely]] {
        throw_exception("Invalid element key");
    }

    const auto selection = calculate_selection();
    return selection.is_selected(element_key);
}

auto SelectionBuilder::set_selection(Selection&& selection) -> void {
    using std::swap;

    operations_.clear();
    swap(initial_selection_, selection);
}

auto SelectionBuilder::bake_selection() -> void {
    set_selection(calculate_selection());
}

auto SelectionBuilder::get_baked_selection() const -> const Selection& {
    if (!operations_.empty()) [[unlikely]] {
        throw_exception("Selection has been modified after baking.");
    }

    return initial_selection_;
}

auto SelectionBuilder::calculate_selected_keys() const -> std::vector<element_key_t> {
    // TODO cache the last selection somehow, then return span
    const auto selection = calculate_selection();
    const auto keys = selection.selected_elements();
    return std::vector<element_key_t>(keys.begin(), keys.end());
}

}  // namespace logicsim
