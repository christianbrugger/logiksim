#include "core/component/editable_circuit/circuit_data.h"

#include "core/allocated_size/std_optional.h"
#include "core/allocated_size/std_vector.h"
#include "core/component/editable_circuit/circuit_data.h"
#include "core/format/container.h"
#include "core/format/std_type.h"
#include "core/logging.h"
#include "core/vocabulary/allocation_info.h"

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

auto CircuitData::allocation_info() const -> CircuitDataAllocInfo {
    return CircuitDataAllocInfo {
        .layout = layout.allocation_info(),
        .index = index.allocation_info(),
        .selection_store = Byte {selection_store.allocated_size()},
        .visible_selection = Byte {visible_selection.allocated_size()},
        .history = Byte {history.allocated_size()},

        .messages = Byte {get_allocated_size(messages)} +
                    Byte {get_allocated_size(message_validator)},
    };
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

}  // namespace editable_circuit

}  // namespace logicsim
