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
 * @brief: Contains complete editable circuit data.
 **/
struct CircuitData {
    Layout layout {};
    LayoutIndex index {};
    SelectionStore selection_store {};
    VisibleSelection visible_selection {};

    std::optional<message_vector_t> messages {std::nullopt};
    std::optional<MessageValidator> message_validator {std::nullopt};

    [[nodiscard]] auto operator==(const CircuitData&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    auto submit(const InfoMessage& message) -> void;
};

static_assert(std::is_aggregate_v<CircuitData>);
static_assert(std::regular<CircuitData>);

}  // namespace editable_circuit

}  // namespace logicsim

#endif
