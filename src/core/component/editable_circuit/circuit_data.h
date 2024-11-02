#ifndef LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_CIRCUIT_DATA_H
#define LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_CIRCUIT_DATA_H

#include "core/component/editable_circuit/layout_index.h"
#include "core/component/editable_circuit/selection_store.h"
#include "core/component/editable_circuit/visible_selection.h"
#include "core/format/enum.h"
#include "core/format/struct.h"
#include "core/layout.h"
#include "core/layout_message_forward.h"
#include "core/layout_message_validator.h"
#include "core/vocabulary/placed_decoration.h"

#include <ankerl/unordered_dense.h>

#include <optional>
#include <vector>

namespace logicsim {

namespace editable_circuit {

enum class UndoType : uint8_t {
    create_temporary_element,
    delete_temporary_element,
};

}

template <>
[[nodiscard]] auto format(editable_circuit::UndoType type) -> std::string;

namespace editable_circuit {

struct DecorationUndoEntry {
    decoration_key_t key;
    point_t position;
    UndoType type;

    [[nodiscard]] auto operator==(const DecorationUndoEntry&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

struct HistoryData {
    std::vector<DecorationUndoEntry> decoration_undo_entries;
    ankerl::unordered_dense::map<decoration_key_t, PlacedDecoration> decoration_graveyard;

    [[nodiscard]] auto operator==(const HistoryData&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;
};

/**
 * @brief: Contains complete editable circuit data.
 **/
struct CircuitData {
    Layout layout {};
    LayoutIndex index {};
    SelectionStore selection_store {};
    VisibleSelection visible_selection {};
    HistoryData history {};

    std::optional<message_vector_t> messages {std::nullopt};
    std::optional<MessageValidator> message_validator {std::nullopt};

    [[nodiscard]] auto operator==(const CircuitData&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    auto submit(const InfoMessage& message) -> void;

    // undo redo interface
    auto swap_and_delete_temporary_decoration(decoration_id_t decoration_id)
        -> decoration_id_t;
    auto add_temporary_decoration(const DecorationDefinition& definition,
                                  point_t position) -> decoration_id_t;
};

static_assert(std::is_aggregate_v<CircuitData>);
static_assert(std::regular<CircuitData>);

}  // namespace editable_circuit

}  // namespace logicsim

#endif
