
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
    : editable_circuit_ {&editable_circuit},
      initial_selection_ {editable_circuit.create_selection()} {}

auto SelectionBuilder::empty() const noexcept -> bool {
    return initial_selection_->empty() && operations_.empty();
}

auto SelectionBuilder::clear() -> void {
    initial_selection_->clear();
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
    switch (function) {
        using enum SelectionFunction;

        case add: {
            selection.add_element(element_id);
            return;
        }
        case substract: {
            selection.remove_element(element_id);
            return;
        }
        case toggle: {
            selection.toggle_element(element_id);
            return;
        }
    }

    throw_exception("Unknown function");
}

auto add_segment_to_selection(segment_t segment, SelectionBuilder::operation_t operation,
                              Selection& selection,
                              const EditableCircuit& editable_circuit) {
    const auto line = editable_circuit.layout()
                          .segment_tree(segment.element_id)
                          .segment(segment.segment_index)
                          .line;
    const auto segment_sel = get_segment_selection(line, operation.rect);

    if (!segment_sel) {
        return;
    }

    switch (operation.function) {
        using enum SelectionFunction;

        case add: {
            selection.add_segment(segment, *segment_sel);
            return;
        }
        case substract: {
            selection.remove_segment(segment, *segment_sel);
            return;
        }
        case toggle: {
            selection.toggle_segment(segment, *segment_sel);
            return;
        }
    }

    throw_exception("Unknown function");
}

auto apply_function(Selection& selection, const EditableCircuit& editable_circuit,
                    SelectionBuilder::operation_t operation) -> void {
    // const auto t = Timer {"apply_function", Timer::Unit::ms, 3};
    const auto selected_elements = editable_circuit.query_selection(operation.rect);

    for (auto&& element : selected_elements) {
        if (element.segment_index == null_segment_index) {
            add_element_to_selection(element.element_id, operation.function, selection,
                                     editable_circuit);
        } else {
            const auto segment = segment_t {element.element_id, element.segment_index};
            add_segment_to_selection(segment, operation, selection, editable_circuit);
        }
    }
}

}  // namespace

auto SelectionBuilder::selection() const -> const Selection& {
    if (cached_selection_) {
        return *cached_selection_;
    }
    if (operations_.empty()) {
        return *initial_selection_;
    }

    cached_selection_ = Selection {};
    // cached_selection_ = editable_circuit_->create_selection();
    auto& selection = *cached_selection_;

    selection = *initial_selection_;
    for (auto&& operation : operations_) {
        apply_function(selection, *editable_circuit_, operation);
    }

    return selection;
}

auto SelectionBuilder::create_selection_mask() const -> selection_mask_t {
    if (empty()) {
        return {};
    }

    // TODO create algorithm to mask?
    const auto element_count = editable_circuit_->schematic().element_count();
    auto mask = selection_mask_t(element_count, false);

    for (element_id_t element_id : selection().selected_elements()) {
        mask.at(element_id.value) = true;
    }

    return mask;
}

auto SelectionBuilder::copy_selection() const -> selection_handle_t {
    return editable_circuit_->create_selection(selection());
}

auto SelectionBuilder::all_operations_applied() const -> bool {
    return operations_.empty();
}

auto SelectionBuilder::apply_all_operations() -> void {
    if (operations_.empty()) {
        return;
    }

    // update cache
    static_cast<void>(selection());

    if (cached_selection_) {
        initial_selection_->swap(*cached_selection_);
    }

    operations_.clear();
    cached_selection_.reset();
}

}  // namespace logicsim
