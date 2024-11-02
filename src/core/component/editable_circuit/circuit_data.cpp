#include "core/component/editable_circuit/circuit_data.h"

#include "core/algorithm/fmt_join.h"
#include "core/allocated_size/ankerl_unordered_dense.h"
#include "core/allocated_size/std_optional.h"
#include "core/allocated_size/std_vector.h"
#include "core/component/editable_circuit/circuit_data.h"
#include "core/format/container.h"
#include "core/logging.h"

namespace logicsim {

namespace editable_circuit {

constexpr static inline auto DEBUG_PRINT_MESSAGES = false;

//
// Circuit Data
//

auto CircuitData::format() const -> std::string {
    return fmt::format(
        "CircuitStore{{\n"
        "layout = {}\n"
        "index = {}\n"
        "selection_store = {}\n"
        "visible_selection = {}\n"
        "history = {}\n"
        "messages = {}\n"
        "message_validator = {}\n"
        "}}\n",
        layout, index, selection_store, visible_selection, history, messages,
        message_validator);
}

auto CircuitData::allocated_size() const -> std::size_t {
    return layout.allocated_size() +             //
           index.allocated_size() +              //
           selection_store.allocated_size() +    //
           visible_selection.allocated_size() +  //
           history.allocated_size() +            //

           get_allocated_size(messages) +  //
           get_allocated_size(message_validator);
}

auto CircuitData::submit(const InfoMessage& message) -> void {
    if constexpr (DEBUG_PRINT_MESSAGES) {
        print(message);
    }

    index.submit(message);
    selection_store.submit(message);
    visible_selection.submit(message);

    if (messages) {
        messages->push_back(message);
    }
    if (message_validator) {
        message_validator->submit(message);
    }
}

//
// Undo Redo Interface
//

}  // namespace editable_circuit

template <>
[[nodiscard]] auto format(editable_circuit::UndoType type) -> std::string {
    using namespace editable_circuit;

    switch (type) {
        using enum UndoType;

        case create_temporary_element:
            return "create_element";
        case delete_temporary_element:
            return "delete_element";
    };
    std::terminate();
}

namespace editable_circuit {

auto DecorationUndoEntry::format() const -> std::string {
    return fmt::format("DecorationUndoEntry(type = {}, key = {}, position = {})", type,
                       key, position);
}

auto HistoryData::format() const -> std::string {
    const auto entry_str = fmt_join(",\n    ", decoration_undo_entries);
    const auto graveyard_str = fmt_join(",\n    ", decoration_graveyard);

    return fmt::format(
        "UndoHistory(\n"
        "  decoration_undo_entries = [\n"
        "    {}\n"
        "  ],\n"
        "  decoration_graveyard = [\n"
        "    {}\n"
        "  ],\n"
        ")",
        entry_str, graveyard_str);
}

auto HistoryData::allocated_size() const -> std::size_t {
    return get_allocated_size(decoration_undo_entries) +  //
           get_allocated_size(decoration_graveyard);
}

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

auto CircuitData::swap_and_delete_temporary_decoration(decoration_id_t decoration_id)
    -> decoration_id_t {
    if (!decoration_id) [[unlikely]] {
        throw std::runtime_error("decoration id is invalid");
    }
    if (layout.decorations().display_state(decoration_id) != display_state_t::temporary)
        [[unlikely]] {
        throw std::runtime_error("can only delete temporary objects");
    }

    // history
    const auto key = index.key_index().get(decoration_id);
    Expects(key);
    history.decoration_undo_entries.emplace_back(DecorationUndoEntry {
        .key = key,
        .position = point_t {},
        .type = UndoType::create_temporary_element,
    });
    history.decoration_graveyard.emplace(key,
                                         to_placed_decoration(layout, decoration_id));

    // delete
    submit(info_message::DecorationDeleted {decoration_id});
    const auto last_id = layout.decorations().swap_and_delete(decoration_id);
    if (decoration_id != last_id) {
        _notify_decoration_id_change(*this, decoration_id, last_id);
    }

    return last_id;
}

auto CircuitData::add_temporary_decoration(const DecorationDefinition& definition,
                                           point_t position) -> decoration_id_t {
    // create
    const auto decoration_id =
        layout.decorations().add(definition, position, display_state_t::temporary);
    submit(info_message::DecorationCreated {decoration_id});

    // history
    const auto key = index.key_index().get(decoration_id);
    Expects(key);
    history.decoration_undo_entries.emplace_back(DecorationUndoEntry {
        .key = key,
        .position = point_t {},
        .type = UndoType::delete_temporary_element,
    });
    print(history);

    return decoration_id;
}

}  // namespace editable_circuit

}  // namespace logicsim
