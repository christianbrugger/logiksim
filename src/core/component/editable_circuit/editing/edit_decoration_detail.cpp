#include "component/editable_circuit/editing/edit_decoration_detail.h"

#include "component/editable_circuit/circuit_data.h"

namespace logicsim {

namespace editable_circuit {

namespace editing {

//
// Decoration Colliding
//

auto is_decoration_colliding(const CircuitData& circuit,
                             const decoration_layout_data_t& data) -> bool {
    static_cast<void>(circuit);
    static_cast<void>(data);
    // return circuit.index.collision_index().is_colliding(data);
    return false;
}

auto is_decoration_colliding(const CircuitData& circuit,
                             decoration_id_t decoration_id) -> bool {
    const auto data = to_decoration_layout_data(circuit.layout, decoration_id);
    return is_decoration_colliding(circuit, data);
}

}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim
