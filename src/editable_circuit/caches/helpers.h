#ifndef LOGIKSIM_EDITABLE_CIRCUIT_CACHES_HELPERS
#define LOGIKSIM_EDITABLE_CIRCUIT_CACHES_HELPERS

#include "circuit.h"
#include "editable_circuit/messages.h"

namespace logicsim {

auto add_element_to_cache(auto &&cache, const Circuit &circuit, element_id_t element_id)
    -> void {
    using namespace editable_circuit::info_message;
    const auto &schematic = circuit.schematic();
    const auto &layout = circuit.layout();

    const auto element = schematic.element(element_id);

    if (element.is_logic_item()) {
        const auto data = to_layout_calculation_data(circuit, element_id);
        cache.submit(LogicItemInserted {element_id, data});
    }

    else if (element.is_wire()) {
        const auto &segment_tree = layout.segment_tree(element_id);
        for (const auto segment_index : segment_tree.indices()) {
            const auto segment = segment_tree.segment(segment_index);
            cache.submit(
                SegmentInserted {segment_t {element_id, segment_index}, segment});
        }
    }
}

auto add_circuit_to_cache(auto &&cache, const Circuit &circuit) -> void {
    const auto &layout = circuit.layout();

    for (const auto element_id : layout.element_ids()) {
        if (is_inserted(layout.display_state(element_id))) {
            add_element_to_cache(cache, circuit, element_id);
        }
    }
}

}  // namespace logicsim

#endif
