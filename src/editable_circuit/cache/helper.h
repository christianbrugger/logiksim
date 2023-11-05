#ifndef LOGIKSIM_EDITABLE_CIRCUIT_CACHE_HELPER_H
#define LOGIKSIM_EDITABLE_CIRCUIT_CACHE_HELPER_H

#include "editable_circuit/message.h"
#include "layout.h"

namespace logicsim {

auto add_logic_item_to_cache(auto &&cache, const Layout &layout,
                             logicitem_id_t logicitem_id) -> void {
    const auto data = to_layout_calculation_data(layout, logicitem_id);
    cache.submit(editable_circuit::info_message::LogicItemInserted {logicitem_id, data});
}

auto add_wire_to_cache(auto &&cache, const Layout &layout, wire_id_t wire_id)
    -> void {
    const auto &segment_tree = layout.wires().segment_tree(wire_id);

    for (const auto segment_index : segment_tree.indices()) {
        cache.submit(editable_circuit::info_message::SegmentInserted {
            .segment = segment_t {wire_id, segment_index},
            .segment_info = segment_tree.info(segment_index)});
    }
}

auto add_logic_items_to_cache(auto &&cache, const Layout &layout) -> void {
    for (const auto logicitem_id : logicitem_ids(layout)) {
        if (is_inserted(layout, logicitem_id)) {
            add_logic_item_to_cache(cache, layout, logicitem_id);
        }
    }
}

auto add_layout_to_cache(auto &&cache, const Layout &layout) -> void {
    add_logic_items_to_cache(cache, layout);

    for (const auto wire_id : inserted_wire_ids(layout)) {
        add_wire_to_cache(cache, layout, wire_id);
    }
}

}  // namespace logicsim

#endif
