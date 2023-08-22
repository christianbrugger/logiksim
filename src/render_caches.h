#ifndef LOGIKSIM_RENDER_CACHES_H
#define LOGIKSIM_RENDER_CACHES_H

#include "render_generic.h"

namespace logicsim {

class EditableCircuit;

auto render_editable_circuit_connection_cache(BLContext& ctx,
                                              const EditableCircuit& editable_circuit,
                                              const RenderSettings& settings) -> void;

auto render_editable_circuit_collision_cache(BLContext& ctx,
                                             const EditableCircuit& editable_circuit,
                                             const RenderSettings& settings) -> void;

auto render_editable_circuit_selection_cache(BLContext& ctx,
                                             const EditableCircuit& editable_circuit,
                                             const RenderSettings& settings) -> void;

}  // namespace logicsim

#endif
