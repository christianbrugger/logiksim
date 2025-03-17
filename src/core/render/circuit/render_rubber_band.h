#ifndef LOGICSIM_CORE_RENDER_CIRCUIT_RENDER_RUBBER_BAND_H
#define LOGICSIM_CORE_RENDER_CIRCUIT_RENDER_RUBBER_BAND_H

namespace logicsim {

struct Context;
class EditableCircuit;

auto render_rubber_band(Context& ctx, const EditableCircuit& editable_circuit) -> void;

}  // namespace logicsim

#endif
