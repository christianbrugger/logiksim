#ifndef LOGIKSIM_EDITABLE_CIRCUIT_CACHE_HELPER_H
#define LOGIKSIM_EDITABLE_CIRCUIT_CACHE_HELPER_H

#include "editable_circuit/message.h"
#include "layout.h"

namespace logicsim {

auto add_logic_item_to_cache(auto &&cache, layout::ConstElement element) -> void {
    const auto data = element.to_layout_calculation_data();
    cache.submit(editable_circuit::info_message::LogicItemInserted {element, data});
}

auto add_wire_to_cache(auto &&cache, layout::ConstElement element) -> void {
    const auto &segment_tree = element.segment_tree();

    for (const auto segment_index : segment_tree.indices()) {
        cache.submit(editable_circuit::info_message::SegmentInserted {
            .segment = segment_t {element, segment_index},
            .segment_info = segment_tree.info(segment_index)});
    }
}

auto add_element_to_cache(auto &&cache, layout::ConstElement element) -> void {
    if (element.is_logic_item()) {
        add_logic_item_to_cache(cache, element);
    } else if (element.is_wire()) {
        add_wire_to_cache(cache, element);
    }
}

auto add_layout_to_cache(auto &&cache, const Layout &layout) -> void {
    for (const auto element : layout.elements()) {
        if (element.is_inserted()) {
            add_element_to_cache(cache, element);
        }
    }
}

auto add_logic_items_to_cache(auto &&cache, const Layout &layout) -> void {
    for (const auto element : layout.elements()) {
        if (element.is_inserted() && element.is_logic_item()) {
            add_logic_item_to_cache(cache, element);
        }
    }
}

}  // namespace logicsim

#endif
