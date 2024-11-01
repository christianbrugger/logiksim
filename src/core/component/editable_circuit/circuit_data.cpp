#include "core/component/editable_circuit/circuit_data.h"

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
    return fmt::format("DecorationUndoEntry(type = {}, uid = {}, position = {})", type,
                       uid, position);
}

auto UndoHistory::format() const -> std::string {
    return fmt::format("UndoHistory(\n  decoration_undo_entries = {}\n)",
                       decoration_undo_entries);
}

auto UndoHistory::allocated_size() const -> std::size_t {
    return get_allocated_size(decoration_undo_entries);
}

auto CircuitData::add_temporary_decoration(const DecorationDefinition& definition,
                                           point_t position) -> decoration_id_t {
    const auto decoration_id =
        layout.decorations().add(definition, position, display_state_t::temporary);
    submit(info_message::DecorationCreated {decoration_id});

    history.decoration_undo_entries.emplace_back(DecorationUndoEntry {
        .uid = 0,
        .position = point_t {},
        .type = UndoType::delete_temporary_element,
    });
    // print(history);
    // print(index.key_index());

    return decoration_id;
}

}  // namespace editable_circuit

}  // namespace logicsim
