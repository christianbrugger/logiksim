#include "core/component/circuit_ui_model/mouse_logic/mouse_logic_status.h"

#include "core/vocabulary/ui_status.h"

#include <fmt/core.h>

namespace logicsim {

namespace circuit_ui_model {

auto mouse_logic_status_t::format() const -> std::string {
    return fmt::format("mouse_logic_status_t(require_repaint = {}, dialogs_changed = {})",
                       require_repaint, dialogs_changed);
}

auto operator|(mouse_logic_status_t lhs, mouse_logic_status_t rhs)
    -> mouse_logic_status_t {
    return mouse_logic_status_t {
        .require_repaint = lhs.require_repaint || rhs.require_repaint,
        .dialogs_changed = lhs.dialogs_changed || rhs.dialogs_changed,
    };
}

auto operator|=(mouse_logic_status_t &lhs, mouse_logic_status_t rhs)
    -> mouse_logic_status_t & {
    lhs = lhs | rhs;
    return lhs;
};

auto operator|=(UIStatus &lhs, const mouse_logic_status_t &rhs) -> UIStatus & {
    lhs.require_repaint |= rhs.require_repaint;
    lhs.dialogs_changed |= rhs.dialogs_changed;
    return lhs;
};

auto mouse_release_status_t::format() const -> std::string {
    return fmt::format("editing_logic_result_t(finished = {}, {})", finished,
                       mouse_logic_status);
}

}  // namespace circuit_ui_model

}  // namespace logicsim
