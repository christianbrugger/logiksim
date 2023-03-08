#include "render_widget.h"

namespace logicsim {

auto SelectionManager::clear() -> void {
    operations_.clear();
}

auto SelectionManager::add(SelectionFunction function, rect_fine_t rect,
                           point_fine_t anchor) -> void {
    operations_.emplace_back(function, rect, anchor);
}

auto SelectionManager::update_last(rect_fine_t rect) -> void {
    if (operations_.empty()) [[unlikely]] {
        throw_exception("Cannot update with empty operations.");
    }
    operations_.back().rect = rect;
}

namespace {
auto apply_function(SelectionManager::selection_mask_t& selection,
                    const EditableCircuit& editable_circuit,
                    SelectionManager::operation_t operation) -> void {
    auto elements = editable_circuit.query_selection(operation.rect);
    std::ranges::sort(elements);

    if (elements.size() == 0) {
        return;
    }

    // bound checking
    if (elements.front().value < 0 || elements.back().value >= selection.size())
        [[unlikely]] {
        throw_exception("Element ids are out of selection bounds.");
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

auto SelectionManager::has_selection() const -> bool {
    return !operations_.empty();
}

auto SelectionManager::create_selection_mask(
    const EditableCircuit& editable_circuit) const -> selection_mask_t {
    if (operations_.empty()) {
        return {};
    }

    const auto element_count = editable_circuit.schematic().element_count();
    auto selection = selection_mask_t(element_count, false);

    for (auto&& operation : operations_) {
        apply_function(selection, editable_circuit, operation);
    }
    return selection;
}

auto SelectionManager::last_anchor_position() const -> std::optional<point_fine_t> {
    if (operations_.empty()) {
        return std::nullopt;
    }
    return operations_.back().anchor;
}

}  // namespace logicsim
