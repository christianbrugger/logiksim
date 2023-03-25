
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
    initial_selected_.clear();
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
auto apply_function(SelectionBuilder::selection_mask_t& selection,
                    const EditableCircuit& editable_circuit,
                    SelectionBuilder::operation_t operation) -> void {
    auto elements = editable_circuit.query_selection(operation.rect);
    std::ranges::sort(elements);

    if (elements.size() == 0) {
        return;
    }

    // bound checking
    if (elements.front().value < 0 || elements.back().value >= std::ssize(selection))
        [[unlikely]] {
        throw_exception("Element ids are out of selection bounds.");
    }

    if (operation.function == SelectionFunction::toggle) {
        for (auto&& element_id : elements) {
            selection[element_id.value] ^= true;
        }
    }

    if (operation.function == SelectionFunction::add) {
        for (auto&& element_id : elements) {
            selection[element_id.value] = true;
        }
    }

    if (operation.function == SelectionFunction::substract) {
        for (auto&& element_id : elements) {
            selection[element_id.value] = false;
        }
    }
}
}  // namespace

auto SelectionBuilder::create_selection_mask() const -> selection_mask_t {
    if (initial_selected_.empty() && operations_.empty()) {
        return {};
    }

    const auto element_count = editable_circuit_.schematic().element_count();
    auto selection = selection_mask_t(element_count, false);

    const auto initial_element_ids = editable_circuit_.to_element_ids(initial_selected_);
    for (element_id_t element_id : initial_element_ids) {
        if (element_id != null_element) {
            selection.at(element_id.value) = true;
        }
    }

    for (auto&& operation : operations_) {
        apply_function(selection, editable_circuit_, operation);
    }
    return selection;
}

auto SelectionBuilder::claculate_item_selected(element_id_t element_id) const -> bool {
    if (element_id < element_id_t {0}) [[unlikely]] {
        throw_exception("Invalid element id");
    }

    const auto selections = create_selection_mask();

    if (element_id.value >= std::ssize(selections)) {
        return false;
    }

    return selections.at(element_id.value);
}

auto SelectionBuilder::set_selection(std::vector<element_key_t>&& selected_keys) -> void {
    using std::swap;

    operations_.clear();
    swap(initial_selected_, selected_keys);
}

auto SelectionBuilder::bake_selection() -> void {
    auto selected_keys = calculate_selected_keys();
    set_selection(std::move(selected_keys));
}

auto SelectionBuilder::get_baked_selection() const -> const std::vector<element_key_t>& {
    if (!operations_.empty()) [[unlikely]] {
        throw_exception("Selection has been modified after baking.");
    }

    return initial_selected_;
}

auto SelectionBuilder::calculate_selected_ids() const -> std::vector<element_id_t> {
    const auto selection = create_selection_mask();
    const auto maximum_id = gsl::narrow<element_id_t::value_type>(selection.size());

    // TODO create algorithm
    auto selected_ids = std::vector<element_id_t> {};
    for (auto i : range(maximum_id)) {
        if (selection[i]) {
            selected_ids.push_back(element_id_t {i});
        }
    };

    return selected_ids;
}

auto SelectionBuilder::calculate_selected_keys() const -> std::vector<element_key_t> {
    const auto selected_ids = calculate_selected_ids();
    const auto selected_keys = editable_circuit_.to_element_keys(selected_ids);

    return selected_keys;
}

}  // namespace logicsim
