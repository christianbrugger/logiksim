#include "component/editable_circuit/circuit_data.h"

namespace logicsim {

namespace editable_circuit {

CircuitData::CircuitData() : CircuitData {Layout {}} {}

CircuitData::CircuitData(Layout&& layout__)
    : layout {std::move(layout__)},
      index {layout},
      selection_store {},
      visible_selection {} {}

auto CircuitData::format() const -> std::string {
    return fmt::format(
        "CircuitStore{{\n"
        "{}\n"
        "{}\n"
        "{}\n"
        "{}\n"
        "}}\n",
        layout, index, selection_store, visible_selection);
}

auto CircuitData::allocated_size() const -> std::size_t {
    return layout.allocated_size() +           //
           index.allocated_size() +            //
           selection_store.allocated_size() +  //
           visible_selection.allocated_size();
}

auto CircuitData::submit(const editable_circuit::InfoMessage& message) -> void {
    index.submit(message);
    selection_store.submit(message);
    visible_selection.submit(message);
}

}  // namespace editable_circuit

}  // namespace logicsim
