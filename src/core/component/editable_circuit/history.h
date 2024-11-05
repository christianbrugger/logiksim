#ifndef LOGICSIM_CORE_COMPONENT_EDITABLE_CIRCUIT_HISTORY_H
#define LOGICSIM_CORE_COMPONENT_EDITABLE_CIRCUIT_HISTORY_H

#include "core/component/editable_circuit/history_stack.h"
#include "core/format/enum.h"
#include "core/format/struct.h"

namespace logicsim {

namespace editable_circuit {

enum class HistoryState : uint8_t {
    disabled,

    track_undo_new,
    track_undo_replay,
    track_redo_replay,
};

}

template <>
[[nodiscard]] auto format(editable_circuit::HistoryState state) -> std::string;

namespace editable_circuit {

struct History {
    HistoryState state {HistoryState::disabled};
    HistoryStack undo_stack {};
    HistoryStack redo_stack {};

    [[nodiscard]] auto operator==(const History&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    [[nodiscard]] auto get_stack() -> HistoryStack*;
};

static_assert(std::regular<History>);

}  // namespace editable_circuit

}  // namespace logicsim

#endif
