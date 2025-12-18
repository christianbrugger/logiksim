//
// Generate all messages equivalent to build the given layout.
//
#ifndef LOGICSIM_LAYOUT_MESSAGE_GENERATION_H
#define LOGICSIM_LAYOUT_MESSAGE_GENERATION_H

#include "core/layout.h"
#include "core/layout_message.h"

namespace logicsim {

//
// Inserted Messages Only
//

auto generate_inserted_logicitem_message(auto &&obj, const Layout &layout,
                                         logicitem_id_t logicitem_id) -> void {
    const auto data = to_layout_calculation_data(layout, logicitem_id);
    obj.submit(info_message::LogicItemInserted {logicitem_id, data});
}

auto generate_inserted_decoration_message(auto &&obj, const Layout &layout,
                                          decoration_id_t decoration_id) -> void {
    const auto data = to_decoration_layout_data(layout, decoration_id);
    obj.submit(info_message::DecorationInserted {decoration_id, data});
}

auto generate_inserted_wire_message(auto &&obj, const Layout &layout, wire_id_t wire_id)
    -> void {
    const auto &segment_tree = layout.wires().segment_tree(wire_id);

    for (const auto segment_index : segment_tree.indices()) {
        obj.submit(info_message::SegmentInserted {
            .segment = segment_t {wire_id, segment_index},
            .segment_info = segment_tree.info(segment_index)});
    }
}

auto generate_inserted_logicitem_messages(auto &&obj, const Layout &layout) -> void {
    for (const auto logicitem_id : logicitem_ids(layout)) {
        if (is_inserted(layout, logicitem_id)) {
            generate_inserted_logicitem_message(obj, layout, logicitem_id);
        }
    }
}

auto generate_inserted_decoration_messages(auto &&obj, const Layout &layout) -> void {
    for (const auto decoration_id : decoration_ids(layout)) {
        if (is_inserted(layout, decoration_id)) {
            generate_inserted_decoration_message(obj, layout, decoration_id);
        }
    }
}

auto generate_inserted_wire_messages(auto &&obj, const Layout &layout) -> void {
    for (const auto wire_id : inserted_wire_ids(layout)) {
        generate_inserted_wire_message(obj, layout, wire_id);
    }
}

auto generate_inserted_layout_messages(auto &&obj, const Layout &layout) -> void {
    generate_inserted_logicitem_messages(obj, layout);
    generate_inserted_decoration_messages(obj, layout);
    generate_inserted_wire_messages(obj, layout);
}

//
// Created Messages
//

auto generate_created_logicitem_message(auto &&obj, logicitem_id_t logicitem_id) -> void {
    obj.submit(info_message::LogicItemCreated {logicitem_id});
}

auto generate_created_decoration_message(auto &&obj, decoration_id_t decoration_id)
    -> void {
    obj.submit(info_message::DecorationCreated {decoration_id});
}

auto generate_created_wire_message(auto &&obj, const Layout &layout, wire_id_t wire_id)
    -> void {
    const auto &segment_tree = layout.wires().segment_tree(wire_id);

    for (const auto segment_index : segment_tree.indices()) {
        obj.submit(info_message::SegmentCreated {
            .segment = segment_t {wire_id, segment_index},
            .size = segment_tree.part(segment_index).end,
        });
    }
}

auto generate_created_logicitem_messages(auto &&obj, const Layout &layout) -> void {
    for (const auto logicitem_id : logicitem_ids(layout)) {
        generate_created_logicitem_message(obj, logicitem_id);
    }
}

auto generate_created_decoration_messages(auto &&obj, const Layout &layout) -> void {
    for (const auto decoration_id : decoration_ids(layout)) {
        generate_created_decoration_message(obj, decoration_id);
    }
}

auto generate_created_wire_messages(auto &&obj, const Layout &layout) -> void {
    for (const auto wire_id : wire_ids(layout)) {
        generate_created_wire_message(obj, layout, wire_id);
    }
}

auto generate_created_layout_messages(auto &&obj, const Layout &layout) -> void {
    generate_created_logicitem_messages(obj, layout);
    generate_created_decoration_messages(obj, layout);
    generate_created_wire_messages(obj, layout);
}

//
// All Messages
//

auto generate_all_logicitem_messages(auto &&obj, const Layout &layout) -> void {
    for (const auto logicitem_id : logicitem_ids(layout)) {
        generate_created_logicitem_message(obj, logicitem_id);

        if (is_inserted(layout, logicitem_id)) {
            generate_inserted_logicitem_message(obj, layout, logicitem_id);
        }
    }
}

auto generate_all_decoration_messages(auto &&obj, const Layout &layout) -> void {
    for (const auto decoration_id : decoration_ids(layout)) {
        generate_created_decoration_message(obj, decoration_id);

        if (is_inserted(layout, decoration_id)) {
            generate_inserted_decoration_message(obj, layout, decoration_id);
        }
    }
}

auto generate_all_wire_messages(auto &&obj, const Layout &layout) -> void {
    for (const auto wire_id : wire_ids(layout)) {
        generate_created_wire_message(obj, layout, wire_id);
    }

    for (const auto wire_id : inserted_wire_ids(layout)) {
        generate_inserted_wire_message(obj, layout, wire_id);
    }
}

auto generate_all_layout_messages(auto &&obj, const Layout &layout) -> void {
    generate_all_logicitem_messages(obj, layout);
    generate_all_decoration_messages(obj, layout);
    generate_all_wire_messages(obj, layout);
}

}  // namespace logicsim

#endif
