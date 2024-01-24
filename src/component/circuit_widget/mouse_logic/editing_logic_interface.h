#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_EDITING_LOGIC_INTERFACE_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_EDITING_LOGIC_INTERFACE_H

namespace logicsim {

class EditableCircuit;

namespace circuit_widget {

/**
 * @brief: Pure interface for editable mouse logic.
 *
 * Note, this class shall not contain any implemented methods.
 */
class EditingLogicInterface {
   public:
    virtual ~EditingLogicInterface() = default;

    /**
     * @brief: This method called before the destructor.
     *
     * Note, all temporary elements & selections need to be freed at this stage.
     * As it is the last time this object sees editable_circuit.
     */
    virtual auto finalize(EditableCircuit& editable_circuit) -> void = 0;
};

}  // namespace circuit_widget

}  // namespace logicsim

#endif
