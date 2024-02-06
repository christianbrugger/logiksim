#ifndef LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_CIRCUIT_DATA_H
#define LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_CIRCUIT_DATA_H

#include "component/editable_circuit/layout_index.h"
#include "component/editable_circuit/selection_store.h"
#include "component/editable_circuit/visible_selection.h"
#include "format/struct.h"
#include "layout.h"
#include "layout_message_forward.h"
#include "layout_message_validator.h"

#include <vector>

namespace logicsim {

namespace editable_circuit {

#ifdef NDEBUG
constexpr static inline auto VALIDATE_MESSAGES_DEFAULT = false;
#else
constexpr static inline auto VALIDATE_MESSAGES_DEFAULT = true;
#endif

struct CircuitDataConfig {
    bool store_messages {false};
    bool validate_messages {VALIDATE_MESSAGES_DEFAULT};
};

/**
 * @brief: Contains all editable circuit data.
 **/
struct CircuitData {
   public:
    [[nodiscard]] explicit CircuitData();
    [[nodiscard]] explicit CircuitData(CircuitDataConfig config);
    [[nodiscard]] explicit CircuitData(Layout&& layout, CircuitDataConfig config = {});

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    auto submit(const InfoMessage& message) -> void;

   public:
    Layout layout;
    LayoutIndex index;
    SelectionStore selection_store;
    VisibleSelection visible_selection;

    std::optional<message_vector_t> messages;
    std::optional<MessageValidator> message_validator;
};

}  // namespace editable_circuit

}  // namespace logicsim

#endif
