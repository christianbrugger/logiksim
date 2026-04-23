#include "core/vocabulary/history_status.h"

#include <fmt/format.h>

namespace logicsim {

auto HistoryStatus::format() const -> std::string {
    return fmt::format("HistoryStatus{{undo_available = {}, redo_available = {}}}",
                       undo_available, redo_available);
}

}  // namespace logicsim
