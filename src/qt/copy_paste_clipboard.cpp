#include "qt/copy_paste_clipboard.h"

#include "base64.h"
#include "editable_circuit.h"
#include "serialize.h"
#include "vocabulary/point.h"

#include <QApplication>
#include <QClipboard>

namespace logicsim {

auto copy_clipboard_selection(const Layout& layout, const Selection& selection,
                              point_t copy_position) -> bool {
    if (!selection.empty()) {
        const auto value =
            base64_encode(serialize_selected(layout, selection, copy_position));

        QApplication::clipboard()->setText(QString::fromStdString(value));
        return true;
    }

    return false;
}

auto copy_clipboard_visible_selection(const EditableCircuit& editable_circuit,
                                      point_t copy_position) -> bool {
    return copy_clipboard_selection(editable_circuit.layout(),
                                    editable_circuit.visible_selection(), copy_position);
}

auto parse_clipboard_data() -> std::optional<serialize::LoadLayoutResult> {
    const auto text = QApplication::clipboard()->text().toStdString();
    const auto binary = base64_decode(text);
    if (binary.empty()) {
        return std::nullopt;
    }
    return load_layout(binary);
}

namespace {

auto insert_clipboard_data_as_temporary(EditableCircuit& editable_circuit,
                                        serialize::LoadLayoutResult&& load_result,
                                        point_t paste_position) {
    const auto guard = SelectionGuard(editable_circuit);
    load_result.add(editable_circuit, serialize::AddParameters {
                                          .insertion_mode = InsertionMode::temporary,
                                          .selection_id = guard.selection_id(),
                                          .load_position = paste_position,
                                      });
    editable_circuit.set_visible_selection(
        editable_circuit.selection(guard.selection_id()));
}

}  // namespace

auto insert_clipboard_data(EditableCircuit& editable_circuit,
                           serialize::LoadLayoutResult&& load_result__,
                           point_t paste_position) -> PasteClipboardResult {
    // insert as temporary
    insert_clipboard_data_as_temporary(editable_circuit, std::move(load_result__),
                                       paste_position);

    // insert as collisions
    auto cross_points__ = editable_circuit.regularize_temporary_selection(
        editable_circuit.visible_selection());
    editable_circuit.split_temporary_before_insert(editable_circuit.visible_selection());
    editable_circuit.change_insertion_mode(editable_circuit.visible_selection(),
                                           InsertionMode::collisions);

    // insert as normal, if possible
    const auto is_colliding = anything_colliding(editable_circuit.visible_selection(),
                                                 editable_circuit.layout());
    if (!is_colliding) {
        editable_circuit.change_insertion_mode(editable_circuit.visible_selection(),
                                               InsertionMode::insert_or_discard);
    }

    return PasteClipboardResult {
        .is_colliding = is_colliding,
        .cross_points = std::move(cross_points__),
    };
}

}  // namespace logicsim
