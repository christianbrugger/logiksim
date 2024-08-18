#ifndef LOGIKSIM_RENDER_CACHES_H
#define LOGIKSIM_RENDER_CACHES_H

#include "render_generic.h"

namespace logicsim {

class EditableCircuit;

auto render_editable_circuit_connection_cache(
    Context& ctx, const EditableCircuit& editable_circuit) -> void;

auto render_editable_circuit_collision_cache(
    Context& ctx, const EditableCircuit& editable_circuit) -> void;

auto render_editable_circuit_selection_cache(
    Context& ctx, const EditableCircuit& editable_circuit) -> void;

}  // namespace logicsim

#endif
