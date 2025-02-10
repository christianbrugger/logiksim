#include "core/component/circuit_ui_model/dialog_manager.h"

namespace logicsim {

namespace circuit_ui_model {

auto DialogManager::close_all(EditableCircuit& editable_circuit) -> void {
    static_cast<void>(editable_circuit);
}

auto DialogManager::run_cleanup(EditableCircuit& editable_circuit) -> void {
    static_cast<void>(editable_circuit);
}

auto DialogManager::size() const -> std::size_t {
    return 0;  // TODO:
}

auto DialogManager::empty() const -> bool {
    return true;  // TODO:
}

auto DialogManager::class_invariant_holds() const -> bool {
    return true;  // TODO:
}

}  // namespace circuit_ui_model

}  // namespace logicsim
