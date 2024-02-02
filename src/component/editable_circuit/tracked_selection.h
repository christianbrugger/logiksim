#ifndef LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_TRACKED_SELECTION_H
#define LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_TRACKED_SELECTION_H

#include "selection.h"

namespace logicsim {

class Layout;

namespace editable_circuit {

/**
 * @brief: A selection that is kept up-to-date by the EditableCircuit across changes.
 *
 * Logic items stay part of the selection in case they are moved, uninserted.
 * Segments tay part of the selection in case of merging or splitting.
 *
 * Pre-conditions:
 *   + requires a correct history of messages of element changes applied.
 *   + no invalid logicitem_id_t or segment_part_t are added to the Selection.
 *
 * Class-invariants:
 *   + all elements in the selection are valid Layout elements
 */
class TrackedSelection {
   public:
    [[nodiscard]] explicit TrackedSelection() = default;
    [[nodiscard]] explicit TrackedSelection(Selection&& selection, const Layout& layout);

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;
    [[nodiscard]] auto operator==(const TrackedSelection&) const -> bool = default;

    [[nodiscard]] auto selection() -> Selection&;
    [[nodiscard]] auto selection() const -> const Selection&;

   private:
    Selection selection_ {};
};

}  // namespace editable_circuit
}  // namespace logicsim

#endif
