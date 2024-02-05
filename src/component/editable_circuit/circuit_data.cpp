#include "component/editable_circuit/circuit_data.h"

#include "allocated_size/std_optional.h"
#include "allocated_size/std_vector.h"
#include "format/container.h"
#include "logging.h"

namespace logicsim {

namespace editable_circuit {

constexpr static inline auto DEBUG_PRINT_MESSAGES = false;

//
// Circuit Data
//

CircuitData::CircuitData() : CircuitData {Layout {}} {}

CircuitData::CircuitData(Layout&& layout__, CircuitDataConfig config)
    : layout {std::move(layout__)},
      index {layout},
      selection_store {},
      visible_selection {},

      store_messages {config.store_messages},
      messages {},
      message_validator {config.validate_messages && true
                             ? std::optional<MessageValidator> {layout}
                             : std::nullopt} {}

auto CircuitData::format() const -> std::string {
    return fmt::format(
        "CircuitStore{{\n"
        "layout = {}\n"
        "index = {}\n"
        "selection_store = {}\n"
        "visible_selection = {}\n"
        "store_messages = {}\n"
        "messages = {}\n"
        "message_validator = {}\n"
        "}}\n",
        layout, index, selection_store, visible_selection, store_messages, messages,
        message_validator);
}

auto CircuitData::allocated_size() const -> std::size_t {
    return layout.allocated_size() +             //
           index.allocated_size() +              //
           selection_store.allocated_size() +    //
           visible_selection.allocated_size() +  //

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

    if (store_messages) {
        messages.push_back(message);
    }
    if (message_validator) {
        message_validator->submit(message);
    }
}

}  // namespace editable_circuit

}  // namespace logicsim
