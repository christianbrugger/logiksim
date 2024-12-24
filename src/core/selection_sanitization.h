/**
 * This modules helps to expand or shrink selections for safe-uninsertion.
 *
 * Selections that end where two different wires are crossing can not be
 * uninsertion. This is because it would leave the circuit in an invalid state,
 * as those two wires cannot be safely merged in all cases, as they both
 * e.g. might have outputs.
 */
#ifndef LOGICSIM_SELECTION_NORMALIZATION_H
#define LOGICSIM_SELECTION_NORMALIZATION_H

#include "core/format/enum.h"

namespace logicsim {

class CollisionIndex;
class Selection;
class Layout;
struct segment_part_t;

namespace editable_circuit {
class Modifier;
}

enum class SanitizeMode {
    expand,
    shrink,
};

template <>
[[nodiscard]] auto format(SanitizeMode mode) -> std::string;

/**
 * @brief: Checks if part can be safely uninserted.
 */
[[nodiscard]] auto is_sanitized(segment_part_t segment_part, const Layout &layout,
                                const CollisionIndex &index) -> bool;

/**
 * @brief: Returns a part that can be safely uninserted.
 */
[[nodiscard]] auto sanitize_part(segment_part_t segment_part, const Layout &layout,
                                 const CollisionIndex &index,
                                 SanitizeMode mode) -> segment_part_t;
[[nodiscard]] auto sanitize_part(segment_part_t segment_part,
                                 const editable_circuit::Modifier &modifier,
                                 SanitizeMode mode) -> segment_part_t;

/**
 * @brief: Returns selection that can be safely uninserted.
 */
auto sanitize_selection(Selection &selection, const Layout &layout,
                        const CollisionIndex &index, SanitizeMode mode) -> void;

}  // namespace logicsim

#endif
