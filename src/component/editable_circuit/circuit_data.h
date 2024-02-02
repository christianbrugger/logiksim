#ifndef LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_CIRCUIT_DATA_H
#define LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_CIRCUIT_DATA_H

#include "component/editable_circuit/layout_index.h"
#include "component/editable_circuit/selection_store.h"
#include "component/editable_circuit/visible_selection.h"
#include "layout.h"
#include "layout_message_forward.h"

namespace logicsim {

namespace editable_circuit {

/**
 * @brief: Contains all editable circuit data.
 **/
struct CircuitData {
   public:
    [[nodiscard]] explicit CircuitData();
    [[nodiscard]] explicit CircuitData(Layout&& layout);

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    auto submit(const editable_circuit::InfoMessage& message) -> void;

   public:
    Layout layout;
    LayoutIndex index;
    SelectionStore selection_store;
    VisibleSelection visible_selection;
};

}  // namespace editable_circuit

}  // namespace logicsim

#endif
