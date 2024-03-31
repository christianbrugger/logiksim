#ifndef LOGICSIM_QT_COPY_PASTE_CLIPBOARD_H
#define LOGICSIM_QT_COPY_PASTE_CLIPBOARD_H

#include "serialize.h"

#include <optional>

namespace logicsim {

struct point_t;
class Layout;
class Selection;
class EditableCircuit;

namespace serialize {
class LoadLayoutResult;
}

/**
 * @brief: Copies the selected elements to the clipboard.
 *
 * Returns true if anything was copied, otherwise the clipboard remains unchanged.
 */
auto copy_clipboard_selection(const Layout& layout, const Selection& selection,
                              point_t copy_position) -> bool;

/**
 * @brief: Copies the visible selected elements to the clipboard.
 *
 * Returns true if anything was copied, otherwise the clipboard remains unchanged.
 */
auto copy_clipboard_visible_selection(const EditableCircuit& editable_circuit,
                                      point_t copy_position) -> bool;

/**
 * @brief: Parses the clipboard data for insertable elements.
 */
[[nodiscard]] auto parse_clipboard_data() -> std::optional<serialize::LoadLayoutResult>;

struct PasteClipboardResult {
    /**
     * @brief: True if any pasted element is in a colliding state.
     */
    bool is_colliding {};

    /**
     * @brief: Contains original cross-points of the pasted data.
     */
    std::vector<point_t> cross_points {};
};

/**
 * @brief: Inserts the parsed clipboard data at the requested position.
 *
 * The result indicates if it was inserted as normal or colliding.
 */
[[nodiscard]] auto insert_clipboard_data(EditableCircuit& editable_circuit,
                                         serialize::LoadLayoutResult&& load_result,
                                         point_t paste_position) -> PasteClipboardResult;

}  // namespace logicsim

#endif
