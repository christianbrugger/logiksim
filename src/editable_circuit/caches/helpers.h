#ifndef LOGIKSIM_EDITABLE_CIRCUIT_CACHES_HELPERS
#define LOGIKSIM_EDITABLE_CIRCUIT_CACHES_HELPERS

#include "circuit.h"
#include "editable_circuit/messages.h"

namespace logicsim {

auto add_element_to_cache(auto &&cache, const Layout &layout, element_id_t element_id)
    -> void {
    using namespace editable_circuit::info_message;
    const auto element = layout.element(element_id);

    if (element.is_logic_item()) {
        const auto data = to_layout_calculation_data(layout, element_id);
        cache.submit(LogicItemInserted {element_id, data});
    }

    else if (element.is_wire()) {
        const auto &segment_tree = layout.segment_tree(element_id);

        for (const auto segment : segment_tree.indices(element_id)) {
            cache.submit(SegmentInserted {segment, get_segment_info(layout, segment)});
        }
    }
}

auto add_layout_to_cache(auto &&cache, const Layout &layout) -> void {
    for (const auto element_id : layout.element_ids()) {
        if (is_inserted(layout.display_state(element_id))) {
            add_element_to_cache(cache, layout, element_id);
        }
    }
}

}  // namespace logicsim

#endif
