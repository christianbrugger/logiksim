#include "core/vocabulary/ui_status.h"

namespace logicsim {

auto UIStatus::format() const -> std::string {
    return fmt::format(
        "UIStatus(repaint_required = {}, config_changed = {}, history_changed = {}, "
        "dialog_changed = {})",
        repaint_required, config_changed, history_changed, dialogs_changed);
}

}  // namespace logicsim
