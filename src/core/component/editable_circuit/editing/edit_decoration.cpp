#include "component/editable_circuit/editing/edit_decoration.h"

#include "component/editable_circuit/circuit_data.h"
#include "component/editable_circuit/editing/edit_decoration_detail.h"
#include "format/struct.h"
#include "geometry/point.h"
#include "layout_info.h"
#include "selection.h"
#include "vocabulary/decoration_layout_data.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>

namespace logicsim {

namespace editable_circuit {

namespace editing {

//
// Delete Decoration
//

namespace {

auto _notify_decoration_id_change(CircuitData& circuit,
                                  const decoration_id_t new_decoration_id,
                                  const decoration_id_t old_decoration_id) {
    circuit.submit(info_message::DecorationIdUpdated {
        .new_decoration_id = new_decoration_id,
        .old_decoration_id = old_decoration_id,
    });

    if (is_inserted(circuit.layout, new_decoration_id)) {
        const auto data = to_decoration_layout_data(circuit.layout, new_decoration_id);

        circuit.submit(info_message::InsertedDecorationIdUpdated {
            .new_decoration_id = new_decoration_id,
            .old_decoration_id = old_decoration_id,
            .data = data,
        });
    }
}

}  // namespace

auto delete_temporary_decoration(CircuitData& circuit, decoration_id_t& decoration_id,
                                 decoration_id_t* preserve_element) -> void {
    if (!decoration_id) [[unlikely]] {
        throw std::runtime_error("decoration id is invalid");
    }

    if (circuit.layout.decorations().display_state(decoration_id) !=
        display_state_t::temporary) [[unlikely]] {
        throw std::runtime_error("can only delete temporary objects");
    }

    circuit.submit(info_message::DecorationDeleted {decoration_id});

    // delete in underlying
    auto last_id = circuit.layout.decorations().swap_and_delete(decoration_id);

    if (decoration_id != last_id) {
        _notify_decoration_id_change(circuit, decoration_id, last_id);
    }

    if (preserve_element != nullptr) {
        if (*preserve_element == decoration_id) {
            *preserve_element = null_decoration_id;
        } else if (*preserve_element == last_id) {
            *preserve_element = decoration_id;
        }
    }

    decoration_id = null_decoration_id;
}

//
// Move Decoration
//

auto is_decoration_position_representable(const Layout& layout,
                                          const decoration_id_t decoration_id, int dx,
                                          int dy) -> bool {
    if (!decoration_id) [[unlikely]] {
        throw std::runtime_error("element id is invalid");
    }

    const auto bounding_rect = layout.decorations().bounding_rect(decoration_id);

    return is_representable(bounding_rect.p0, dx, dy) &&
           is_representable(bounding_rect.p1, dx, dy);
}

auto are_decoration_positions_representable(const Layout& layout,
                                            const Selection& selection, int delta_x,
                                            int delta_y) -> bool {
    const auto decoration_valid = [&](decoration_id_t decoration_id) {
        return is_decoration_position_representable(layout, decoration_id, delta_x,
                                                    delta_y);
    };

    return std::ranges::all_of(selection.selected_decorations(), decoration_valid);
}

auto move_temporary_decoration_unchecked(Layout& layout,
                                         const decoration_id_t decoration_id, int dx,
                                         int dy) -> void {
    assert(std::as_const(layout).decorations().display_state(decoration_id) ==
           display_state_t::temporary);
    assert(is_decoration_position_representable(layout, decoration_id, dx, dy));

    const auto position =
        add_unchecked(layout.decorations().position(decoration_id), dx, dy);
    layout.decorations().set_position(decoration_id, position);
}

auto move_or_delete_temporary_decoration(CircuitData& circuit,
                                         decoration_id_t& decoration_id, int dx,
                                         int dy) -> void {
    if (circuit.layout.decorations().display_state(decoration_id) !=
        display_state_t::temporary) [[unlikely]] {
        throw std::runtime_error("Only temporary items can be freely moved.");
    }

    if (!is_decoration_position_representable(circuit.layout, decoration_id, dx, dy)) {
        delete_temporary_decoration(circuit, decoration_id);
        return;
    }

    move_temporary_decoration_unchecked(circuit.layout, decoration_id, dx, dy);
}

//
// Change Insertion Mode
//

namespace {

auto _decoration_change_temporary_to_colliding(
    CircuitData& circuit, const decoration_id_t decoration_id) -> void {
    if (circuit.layout.decorations().display_state(decoration_id) !=
        display_state_t::temporary) [[unlikely]] {
        throw std::runtime_error("element is not in the right state.");
    }

    if (is_decoration_colliding(circuit, decoration_id)) {
        circuit.layout.decorations().set_display_state(decoration_id,
                                                       display_state_t::colliding);
    }

    else {
        circuit.layout.decorations().set_display_state(decoration_id,
                                                       display_state_t::valid);
        circuit.submit(info_message::DecorationInserted {
            .decoration_id = decoration_id,
            .data = to_decoration_layout_data(circuit.layout, decoration_id),
        });
    }
};

auto _decoration_change_colliding_to_insert(CircuitData& circuit,
                                            decoration_id_t& decoration_id) -> void {
    const auto display_state = circuit.layout.decorations().display_state(decoration_id);

    if (display_state == display_state_t::valid) {
        circuit.layout.decorations().set_display_state(decoration_id,
                                                       display_state_t::normal);
        return;
    }

    if (display_state == display_state_t::colliding) [[likely]] {
        // we can only delete temporary elements
        circuit.layout.decorations().set_display_state(decoration_id,
                                                       display_state_t::temporary);
        delete_temporary_decoration(circuit, decoration_id);
        return;
    }

    throw std::runtime_error("element is not in the right state.");
};

auto _decoration_change_insert_to_colliding(Layout& layout,
                                            const decoration_id_t decoration_id) -> void {
    if (layout.decorations().display_state(decoration_id) != display_state_t::normal)
        [[unlikely]] {
        throw std::runtime_error("element is not in the right state.");
    }

    layout.decorations().set_display_state(decoration_id, display_state_t::valid);
};

auto _decoration_change_colliding_to_temporary(
    CircuitData& circuit, const decoration_id_t decoration_id) -> void {
    const auto display_state = circuit.layout.decorations().display_state(decoration_id);

    if (display_state == display_state_t::valid) {
        circuit.submit(info_message::DecorationUninserted {
            .decoration_id = decoration_id,
            .data = to_decoration_layout_data(circuit.layout, decoration_id),
        });

        circuit.layout.decorations().set_display_state(decoration_id,
                                                       display_state_t::temporary);
        return;
    }

    if (display_state == display_state_t::colliding) {
        circuit.layout.decorations().set_display_state(decoration_id,
                                                       display_state_t::temporary);
        return;
    }

    throw std::runtime_error("element is not in the right state.");
};

}  // namespace

auto change_decoration_insertion_mode(CircuitData& circuit,
                                      decoration_id_t& decoration_id,
                                      InsertionMode new_mode) -> void {
    if (!decoration_id) [[unlikely]] {
        throw std::runtime_error("element id is invalid");
    }

    const auto old_mode =
        to_insertion_mode(circuit.layout.decorations().display_state(decoration_id));
    if (old_mode == new_mode) {
        return;
    }

    if (old_mode == InsertionMode::temporary) {
        _decoration_change_temporary_to_colliding(circuit, decoration_id);
    }
    if (new_mode == InsertionMode::insert_or_discard) {
        _decoration_change_colliding_to_insert(circuit, decoration_id);
    }
    if (old_mode == InsertionMode::insert_or_discard) {
        _decoration_change_insert_to_colliding(circuit.layout, decoration_id);
    }
    if (new_mode == InsertionMode::temporary) {
        _decoration_change_colliding_to_temporary(circuit, decoration_id);
    }
}

//
// Add decoration
//

auto add_decoration(CircuitData& circuit, const DecorationDefinition& definition,
                    point_t position, InsertionMode insertion_mode) -> decoration_id_t {
    if (!is_representable(to_decoration_layout_data(definition, position))) {
        return null_decoration_id;
    }
    auto decoration_id = circuit.layout.decorations().add(definition, position,
                                                          display_state_t::temporary);
    circuit.submit(info_message::DecorationCreated {decoration_id});

    if (decoration_id) {
        change_decoration_insertion_mode(circuit, decoration_id, insertion_mode);
    }
    return decoration_id;
}

}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim
