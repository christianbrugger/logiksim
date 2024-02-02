#ifndef LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_PLACEHOLDER_H
#define LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_PLACEHOLDER_H

#include "layout.h"
#include "component/editable_circuit/layout_index.h"
#include "component/editable_circuit/selection_store.h"
#include "component/editable_circuit/visible_selection.h"

namespace logicsim {

namespace editable_circuit {

/**
 * @brief: Allows a Layout to be edited such that it forms a valid circuit.
 * 
 * Class-invariants:
 *   Logic Items:
 *      + Body is fully representable within the grid.
 *   Inserted Logic Items:
 *      + Are not colliding with anything.
 *      + All connections with wires are compatible (type & orientation).
 *   Inserted Wires:
 *      + Segments are not colliding with anything.
 *      + Segments form a flat tree. With input at the root.
 *      + Have either zero or one input.
 *      + Input corresponds to logicitem output and has correct orientation / position.
 *      + Have correctly set SegmentPointTypes (input, output, corner, cross, shadow).
 *   Uninserted Wires:
 *      + Have no valid parts.
 *      + Have no inputs or outputs.
 *      + All SegmentPointTypes are shadow_point
 * 
 *   Layout Index:
 *      + LayoutIndex is always in sync with Layout.
 *   Selections:
 *      + All Elements in all Selections of the SelectionStore are present in Layout.
 *      + Elements in Visible Selection are present in Layout.
 **/
class Placeholder {
   public:
    [[nodiscard]] explicit Placeholder();
    [[nodiscard]] explicit Placeholder(Layout&& layout);


   private:
       Layout layout_;
       LayoutIndex layout_index_;
       // TODO more efficient initialization with Layout
       SelectionStore selection_store_;
       VisibleSelection visible_selection_;
};

}  // namespace editable_circuit

}  // namespace logicsim

#endif
