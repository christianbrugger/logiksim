#ifndef LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_CIRCUIT_DATA_H
#define LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_CIRCUIT_DATA_H

#include "component/editable_circuit/layout_index.h"
#include "component/editable_circuit/selection_store.h"
#include "component/editable_circuit/visible_selection.h"
#include "format/struct.h"
#include "layout.h"
#include "layout_message_forward.h"
#include "layout_message_validator.h"

#include <optional>
#include <vector>

namespace logicsim {

namespace editable_circuit {

/**
 * @brief: Contains all editable circuit data.
 **/
struct CircuitData {
   public:
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    auto submit(const InfoMessage& message) -> void;

   public:
    Layout layout {};
    LayoutIndex index {};
    SelectionStore selection_store {};
    VisibleSelection visible_selection {};

    std::optional<message_vector_t> messages {std::nullopt};
    std::optional<MessageValidator> message_validator {std::nullopt};
};

static_assert(std::is_aggregate_v<CircuitData>);

}  // namespace editable_circuit

}  // namespace logicsim

#endif
