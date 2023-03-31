#ifndef LOGIKSIM_CIRCUIT_H
#define LOGIKSIM_CIRCUIT_H

#include "layout.h"
#include "schematic.h"

namespace logicsim {

// [](element_id_t element_id, layout_calculation_data_t data){..}
// [](element_id_t element_id, segment_info_t segment, segment_index_t segment_index){..}
template <typename ElementCallback, typename SegmentCallback>
auto iter_circuit_elements(const Layout& layout, const Schematic& schematic,
                           ElementCallback element_callback,
                           SegmentCallback segment_callback) -> void {
    for (const auto element : schematic.elements()) {
        const auto element_id = element.element_id();
        if (is_inserted(layout.display_state(element_id))) {
            if (element.is_element()) {
                const auto data
                    = to_layout_calculation_data(schematic, layout, element_id);
                element_callback(element_id, data);
            }
            if (element.is_wire()) {
                const auto& segment_tree = layout.segment_tree(element_id);
                for (const auto segment_index : segment_tree.indices()) {
                    const auto segment = segment_tree.segment(segment_index);
                    segment_callback(element_id, segment, segment_index);
                }
            }
        }
    }
}

}  // namespace logicsim

#endif
