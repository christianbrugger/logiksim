#ifndef LOGICSIM_CORE_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_DECORATION_DETAIL_H
#define LOGICSIM_CORE_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_DECORATION_DETAIL_H

namespace logicsim {

struct decoration_id_t;
struct decoration_layout_data_t;

namespace editable_circuit {

struct CircuitData;

namespace editing {

//
// Decoration Colliding
//

auto is_decoration_colliding(const CircuitData& circuit,
                             const decoration_layout_data_t& data) -> bool;

auto is_decoration_colliding(const CircuitData& circuit, decoration_id_t decoration_id)
    -> bool;

}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim

#endif
