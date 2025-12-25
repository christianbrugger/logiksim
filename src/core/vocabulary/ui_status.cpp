#include "core/vocabulary/ui_status.h"

namespace logicsim {

auto UIStatus::format() const -> std::string {
    return fmt::format(
        "UIStatus(require_repaint = {}, config_changed = {}, history_changed = {}, "
        "dialog_changed = {}, filename_changed)",
        require_repaint, config_changed, history_changed, dialogs_changed,
        filename_changed);
}

}  // namespace logicsim
