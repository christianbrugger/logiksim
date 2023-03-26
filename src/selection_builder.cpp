
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

auto SelectionBuilder::empty() const noexcept -> bool {
    return initial_selection_.empty() && operations_.empty();
}

auto SelectionBuilder::clear() -> void {
    initial_selection_.clear();
    operations_.clear();
    cached_selection_.reset();
}

auto SelectionBuilder::add(SelectionFunction function, rect_fine_t rect) -> void {
    operations_.emplace_back(operation_t {function, rect});
    cached_selection_.reset();
}

auto SelectionBuilder::update_last(rect_fine_t rect) -> void {
    if (operations_.empty()) [[unlikely]] {
        throw_exception("Cannot update last with no operations.");
    }
    if (operations_.back().rect == rect) {
        return;
    }

    operations_.back().rect = rect;
    cached_selection_.reset();
}

auto SelectionBuilder::pop_last() -> void {
    if (operations_.empty()) [[unlikely]] {
        throw_exception("Cannot remove last with no operations.");
    }

    operations_.pop_back();
    cached_selection_.reset();
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

auto SelectionBuilder::calculate_selection() const -> const Selection& {
    if (operations_.empty()) {
        return initial_selection_;
    }
    if (cached_selection_.has_value()) {
        return *cached_selection_;
    }

    auto selection = Selection {initial_selection_};
    for (auto&& operation : operations_) {
        apply_function(selection, editable_circuit_, operation);
    }

    cached_selection_->swap(selection);
    return *cached_selection_;
}

auto SelectionBuilder::create_selection_mask() const -> selection_mask_t {
    if (empty()) {
        return {};
    }

    const auto selected_ids
        = editable_circuit_.to_element_ids(calculate_selection().selected_elements());

    // TODO create algorithm to mask?
    const auto element_count = editable_circuit_.schematic().element_count();
    auto mask = selection_mask_t(element_count, false);

    for (element_id_t element_id : selected_ids) {
        mask.at(element_id.value) = true;
    }

    return mask;
}

auto SelectionBuilder::claculate_is_item_selected(element_key_t element_key) const
    -> bool {
    if (element_key < element_key_t {0}) [[unlikely]] {
        throw_exception("Invalid element key");
    }

    return calculate_selection().is_selected(element_key);
}

auto SelectionBuilder::set_selection(Selection&& selection) -> void {
    using std::swap;

    swap(initial_selection_, selection);

    operations_.clear();
    cached_selection_.reset();
}

auto SelectionBuilder::bake_selection() -> void {
    static_cast<void>(calculate_selection());

    auto temp = Selection {};
    temp.swap(*cached_selection_);
    cached_selection_.reset();

    set_selection(std::move(temp));
}

auto SelectionBuilder::get_baked_selection() const -> const Selection& {
    if (!operations_.empty()) [[unlikely]] {
        throw_exception("Selection has been modified after baking.");
    }

    return initial_selection_;
}

auto SelectionBuilder::calculate_selected_keys() const -> std::span<const element_key_t> {
    return calculate_selection().selected_elements();
}

}  // namespace logicsim
