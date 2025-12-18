#ifndef LOGIKSIM_CORE_RENDER_CIRCUIT_RENDER_LAYOUT_INDEX_H
#define LOGIKSIM_CORE_RENDER_CIRCUIT_RENDER_LAYOUT_INDEX_H

namespace logicsim {

class CollisionIndex;
class SpatialIndex;
class LayoutIndex;
class EditableCircuit;

struct Context;

auto render_layout_connection_index(Context& ctx, const LayoutIndex& index) -> void;
auto render_layout_connection_index(Context& ctx, const EditableCircuit& editable_circuit)
    -> void;

auto render_layout_collision_index(Context& ctx, const CollisionIndex& collision_index)
    -> void;
auto render_layout_collision_index(Context& ctx, const EditableCircuit& editable_circuit)
    -> void;

auto render_layout_selection_index(Context& ctx, const SpatialIndex& selection_index)
    -> void;
auto render_layout_selection_index(Context& ctx, const EditableCircuit& editable_circuit)
    -> void;

}  // namespace logicsim

#endif
