#include "component/editable_circuit/circuit_data.h"
#include "format/container.h"
#include "logging.h"

namespace logicsim {

namespace editable_circuit {

constexpr static inline auto DEBUG_PRINT_MESSAGES = false;

//
// Circuit Data
//

CircuitData::CircuitData() : CircuitData {Layout {}} {}

CircuitData::CircuitData(Layout&& layout__)
    : layout {std::move(layout__)},
      index {layout},
      selection_store {},
      visible_selection {},
      messages {},
      store_messages {false} {}

auto CircuitData::format() const -> std::string {
    return fmt::format(
        "CircuitStore{{\n"
        "layout = {}\n"
        "index = {}\n"
        "selection_store = {}\n"
        "visible_selection = {}\n"
        "messages = {}\n"
        "store_messages = {}\n"
        "}}\n",
        layout, index, selection_store, visible_selection, messages, store_messages);
}

auto CircuitData::allocated_size() const -> std::size_t {
    return layout.allocated_size() +           //
           index.allocated_size() +            //
           selection_store.allocated_size() +  //
           visible_selection.allocated_size();
}

auto CircuitData::submit(const editable_circuit::InfoMessage& message) -> void {
    if constexpr (DEBUG_PRINT_MESSAGES) {
        print(message);
    }

    index.submit(message);
    selection_store.submit(message);
    visible_selection.submit(message);

    if (store_messages) {
        messages.push_back(message);
    }
}

}  // namespace editable_circuit

}  // namespace logicsim
