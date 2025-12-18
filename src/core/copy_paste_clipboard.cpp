#include "core/copy_paste_clipboard.h"

#include "core/base64.h"
#include "core/editable_circuit.h"
#include "core/serialize.h"
#include "core/vocabulary/point.h"
#include "core/vocabulary/save_format.h"

#include <fmt/core.h>

namespace logicsim {

auto layout_to_clipboard_text(const Layout& layout, point_t copy_position)
    -> std::string {
    if (!layout.empty()) {
        return serialize_all(layout, SerializeConfig {
                                         .save_format = SaveFormat::base64_gzip,
                                         .save_position = copy_position,
                                     });
    }
    return std::string {};
}

auto selection_to_clipboard_text(const Layout& layout, const Selection& selection,
                                 point_t copy_position) -> std::string {
    if (!selection.empty()) {
        return serialize_selected(layout, selection,
                                  SerializeConfig {
                                      .save_format = SaveFormat::base64_gzip,
                                      .save_position = copy_position,
                                  });
    }
    return std::string {};
}

auto visible_selection_to_clipboard_text(const EditableCircuit& editable_circuit,
                                         point_t copy_position) -> std::string {
    return selection_to_clipboard_text(
        editable_circuit.layout(), editable_circuit.visible_selection(), copy_position);
}

auto PasteClipboardResult::format() const -> std::string {
    return fmt::format(
        "PasteClipboardResult(\n"  //
        "  is_colliding = {},\n"   //
        "  cross_points = {}\n"    //
        ")",                       //
        is_colliding, cross_points);
}

auto parse_clipboard_text(const std::string& text)
    -> tl::expected<serialize::LoadLayoutResult, LoadError> {
    return load_layout(text);
}

namespace {

auto insert_clipboard_data_as_temporary(EditableCircuit& editable_circuit,
                                        const serialize::LoadLayoutResult& load_result,
                                        point_t paste_position) {
    const auto guard = SelectionGuard(editable_circuit);
    load_result.add_to(editable_circuit, serialize::AddParameters {
                                             .insertion_mode = InsertionMode::temporary,
                                             .selection_id = guard.selection_id(),
                                             .load_position = paste_position,
                                         });
    editable_circuit.set_visible_selection(
        editable_circuit.selection(guard.selection_id()));
}

}  // namespace

auto insert_clipboard_data(EditableCircuit& editable_circuit,
                           const serialize::LoadLayoutResult& load_result,
                           point_t paste_position) -> PasteClipboardResult {
    // insert as temporary
    insert_clipboard_data_as_temporary(editable_circuit, load_result, paste_position);

    // insert as collisions
    auto cross_points_ = editable_circuit.regularize_temporary_selection(
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
        .cross_points = std::move(cross_points_),
    };
}

}  // namespace logicsim
