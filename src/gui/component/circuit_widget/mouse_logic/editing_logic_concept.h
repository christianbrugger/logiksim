#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_EDITING_LOGIC_CONCEPT_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_EDITING_LOGIC_CONCEPT_H

#include <concepts>

namespace logicsim {

class EditableCircuit;

namespace circuit_widget {

template <class T>
concept has_mouse_logic_finalize = requires(T obj, EditableCircuit& editable_circuit) {
    /**
     * @brief: Finalize the operations on editable_circuit.
     *
     * Note, this is the last time this object sees editable_circuit.
     * All temporary elements & selections need to be freed at this stage.
     */
    { obj.finalize(editable_circuit) } -> std::same_as<void>;
};

}  // namespace circuit_widget

}  // namespace logicsim

#endif
