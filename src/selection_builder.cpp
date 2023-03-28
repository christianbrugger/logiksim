
#include "selection_builder.h"

#include "algorithm.h"
#include "editable_circuit.h"
#include "format.h"
#include "range.h"
#include "timer.h"

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

auto add_element_to_selection(element_id_t element_id, SelectionFunction function,
                              Selection& selection,
                              const EditableCircuit& editable_circuit) {
    const auto element_key = editable_circuit.to_element_key(element_id);

    switch (function) {
        using enum SelectionFunction;

        case add: {
            selection.add_element(element_key);
            return;
        }
        case substract: {
            selection.remove_element(element_key);
            return;
        }
        case toggle: {
            selection.toggle_element(element_key);
            return;
        }
    }

    throw_exception("Unknown function");
}

auto add_segment_to_selection(element_id_t element_id, segment_index_t segment_index,
                              SelectionBuilder::operation_t operation,
                              Selection& selection,
                              const EditableCircuit& editable_circuit) {
    const auto line
        = editable_circuit.layout().segment_tree(element_id).segment(segment_index).line;
    const auto segment_sel = get_segment_selection(line, operation.rect);

    if (!segment_sel) {
        return;
    }
    const auto element_key = editable_circuit.to_element_key(element_id);

    switch (operation.function) {
        using enum SelectionFunction;

        case add: {
            selection.add_segment(element_key, segment_index, *segment_sel);
            return;
        }
        case substract: {
            selection.remove_segment(element_key, segment_index, *segment_sel);
            return;
        }
        case toggle: {
            selection.toggle_segment(element_key, segment_index, *segment_sel);
            return;
        }
    }

    throw_exception("Unknown function");
}

auto apply_function(Selection& selection, const EditableCircuit& editable_circuit,
                    SelectionBuilder::operation_t operation) -> void {
    const auto selected_elements = editable_circuit.query_selection2(operation.rect);

    for (auto&& element : selected_elements) {
        if (element.segment_index == null_segment_index) {
            add_element_to_selection(element.element_id, operation.function, selection,
                                     editable_circuit);
        } else {
            add_segment_to_selection(element.element_id, element.segment_index, operation,
                                     selection, editable_circuit);
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
