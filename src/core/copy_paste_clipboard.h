#ifndef LOGICSIM_COPY_PASTE_CLIPBOARD_H
#define LOGICSIM_COPY_PASTE_CLIPBOARD_H

#include "core/format/struct.h"
#include "core/serialize.h"
#include "core/vocabulary/load_error.h"

#include <tl/expected.hpp>

#include <optional>
#include <string>
#include <string_view>

namespace logicsim {

struct point_t;
class Layout;
class Selection;
class EditableCircuit;

namespace serialize {
class LoadLayoutResult;
}

/**
 * @brief: Returns text representation of all items for copy & pasting.
 *
 * Throws if any item is not in display_state_t::normal.
 *
 * Returns an empty string if nothing is selected.
 */
auto layout_to_clipboard_text(const Layout& layout, point_t copy_position) -> std::string;

/**
 * @brief: Returns text representation of selected items for copy & pasting.
 *
 * Throws if any item is not in display_state_t::normal.
 *
 * Returns an empty string if nothing is selected.
 */
auto selection_to_clipboard_text(const Layout& layout, const Selection& selection,
                                 point_t copy_position) -> std::string;

/**
 * @brief: Copies the visible selected elements to the clipboard.
 *
 * Throws if any item is not in display_state_t::normal.
 *
 * Returns an empty string if nothing is selected.
 */
auto visible_selection_to_clipboard_text(const EditableCircuit& editable_circuit,
                                         point_t copy_position) -> std::string;

struct PasteClipboardResult {
    /**
     * @brief: True if any pasted element is in a colliding state.
     */
    bool is_colliding {};

    /**
     * @brief: Contains original cross-points of the pasted data.
     */
    std::vector<point_t> cross_points {};

    [[nodiscard]] auto operator==(const PasteClipboardResult&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

/**
 * @brief: Parses the clipboard data for insertable elements.
 */
[[nodiscard]] auto parse_clipboard_text(std::string_view text)
    -> tl::expected<serialize::LoadLayoutResult, LoadError>;

/**
 * @brief: Inserts the parsed clipboard data at the requested position.
 *
 * The result indicates if it was inserted as normal or colliding.
 */
[[nodiscard]] auto insert_clipboard_data(EditableCircuit& editable_circuit,
                                         const serialize::LoadLayoutResult& load_result,
                                         point_t paste_position) -> PasteClipboardResult;

}  // namespace logicsim

#endif
