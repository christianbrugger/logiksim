
#include "selection_builder.h"

#include "algorithm.h"
#include "editable_circuit.h"
#include "format.h"
#include "range.h"

namespace logicsim {

//
// Selection Builder
//

SelectionBuilder::SelectionBuilder(const EditableCircuit& editable_circuit)
    : editable_circuit_ {&editable_circuit} {}

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

auto SelectionBuilder::selection() const -> const Selection& {
    if (cached_selection_.has_value()) {
        return *cached_selection_;
    }
    if (operations_.empty()) {
        return initial_selection_;
    }

    auto selection = Selection {initial_selection_};
    for (auto&& operation : operations_) {
        apply_function(selection, *editable_circuit_, operation);
    }

    cached_selection_.emplace(std::move(selection));
    return cached_selection_.value();
}

auto SelectionBuilder::create_selection_mask() const -> selection_mask_t {
    if (empty()) {
        return {};
    }
    const auto selected_ids
        = editable_circuit_->to_element_ids(selection().selected_elements());

    // TODO create algorithm to mask?
    const auto element_count = editable_circuit_->schematic().element_count();
    auto mask = selection_mask_t(element_count, false);

    for (element_id_t element_id : selected_ids) {
        mask.at(element_id.value) = true;
    }

    return mask;
}

auto SelectionBuilder::is_selection_baked() const -> bool {
    return operations_.empty();
}

auto SelectionBuilder::bake_selection() -> void {
    // update cache
    static_cast<void>(selection());

    if (cached_selection_.has_value()) {
        initial_selection_.swap(*cached_selection_);
    }

    operations_.clear();
    cached_selection_.reset();
}

auto SelectionBuilder::remove_invalid_element_keys() -> void {
    cached_selection_.reset();

    for (element_key_t element_key : initial_selection_.selected_elements()) {
        if (!editable_circuit_->element_key_valid(element_key)) {
            initial_selection_.remove_element(element_key);
        }
    }
}

}  // namespace logicsim
