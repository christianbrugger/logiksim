#ifndef LOGICSIM_CORE_VOCABULARY_HISTORY_STATUS_H
#define LOGICSIM_CORE_VOCABULARY_HISTORY_STATUS_H

#include "core/format/struct.h"

#include <concepts>

namespace logicsim {

struct HistoryStatus {
    bool undo_available {false};
    bool redo_available {false};

    [[nodiscard]] auto operator==(const HistoryStatus&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::regular<HistoryStatus>);

}  // namespace logicsim

#endif
