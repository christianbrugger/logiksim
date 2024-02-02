#ifndef LOGICSIM_LAYOUT_MESSAGE_GENERATION_H
#define LOGICSIM_LAYOUT_MESSAGE_GENERATION_H

#include "layout.h"
#include "layout_message.h"

namespace logicsim {

auto generate_logicitem_messages(auto &&obj, const Layout &layout,
                             logicitem_id_t logicitem_id) -> void {
    const auto data = to_layout_calculation_data(layout, logicitem_id);
    obj.submit(editable_circuit::info_message::LogicItemInserted {logicitem_id, data});
}

auto generate_wire_messages(auto &&obj, const Layout &layout, wire_id_t wire_id)
    -> void {
    const auto &segment_tree = layout.wires().segment_tree(wire_id);

    for (const auto segment_index : segment_tree.indices()) {
        obj.submit(editable_circuit::info_message::SegmentInserted {
            .segment = segment_t {wire_id, segment_index},
            .segment_info = segment_tree.info(segment_index)});
    }
}

auto generate_logicitem_messages(auto &&obj, const Layout &layout) -> void {
    for (const auto logicitem_id : logicitem_ids(layout)) {
        if (is_inserted(layout, logicitem_id)) {
            generate_logicitem_messages(obj, layout, logicitem_id);
        }
    }
}

auto generate_wire_messages(auto &&obj, const Layout &layout) -> void {
    for (const auto wire_id : inserted_wire_ids(layout)) {
        generate_wire_messages(obj, layout, wire_id);
    }
}

auto generate_layout_messages(auto &&obj, const Layout &layout) -> void {
    generate_logicitem_messages(obj, layout);
    generate_wire_messages(obj, layout);
}

}  // namespace logicsim

#endif
