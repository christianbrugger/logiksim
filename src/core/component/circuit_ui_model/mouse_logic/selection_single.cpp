#include "core/component/circuit_ui_model/mouse_logic/selection_single.h"

#include "core/editable_circuit.h"
#include "core/vocabulary/point_fine.h"

namespace logicsim {

namespace circuit_ui_model {

namespace {

auto add_selection(Selection& selection, const Layout& layout,
                   std::span<const SpatialIndex::value_t> items,
                   point_fine_t point) -> void {
    for (const auto& item : items) {
        if (item.is_logicitem()) {
            selection.add_logicitem(item.logicitem());
        } else if (item.is_segment()) {
            add_segment_part(selection, layout, item.segment(), point);
        } else if (item.is_decoration()) {
            selection.add_decoration(item.decoration());
        }
    }
}

auto remove_selection(Selection& selection, const Layout& layout,
                      std::span<const SpatialIndex::value_t> items,
                      point_fine_t point) -> void {
    for (const auto& item : items) {
        if (item.is_logicitem()) {
            selection.remove_logicitem(item.logicitem());
        } else if (item.is_segment()) {
            remove_segment_part(selection, layout, item.segment(), point);
        } else if (item.is_decoration()) {
            selection.remove_decoration(item.decoration());
        }
    }
}

auto add_whole_trees(Selection& selection, const Layout& layout,
                     std::span<const SpatialIndex::value_t> items) -> void {
    for (const auto& item : items) {
        if (item.is_segment()) {
            add_segment_tree(selection, item.segment().wire_id, layout);
        }
    }
}

auto remove_whole_trees(Selection& selection, const Layout& layout,
                        std::span<const SpatialIndex::value_t> items) -> void {
    for (const auto& item : items) {
        if (item.is_segment()) {
            remove_segment_tree(selection, item.segment().wire_id, layout);
        }
    }
}

}  // namespace

auto SelectionSingleLogic::mouse_press(EditableCircuit& editable_circuit,
                                       point_fine_t point, bool double_click) -> void {
    const auto& layout = editable_circuit.layout();

    const auto items = editable_circuit.query_selection(rect_fine_t {point, point});

    if (items.empty()) {
        return;
    }

    auto selection = Selection {editable_circuit.visible_selection()};

    if (!double_click) {
        if (!all_selected(items, point, selection, layout)) {
            add_selection(selection, layout, items, point);
        } else {
            remove_selection(selection, layout, items, point);
        }
    } else {
        if (!all_selected(items, point, selection, layout)) {
            remove_whole_trees(selection, layout, items);
        } else {
            add_whole_trees(selection, layout, items);
        }
    }

    editable_circuit.set_visible_selection(selection);
}

auto SelectionSingleLogic::finalize(EditableCircuit& editable_circuit) -> void {
    editable_circuit.finish_undo_group();
}

}  // namespace circuit_ui_model

}  // namespace logicsim
