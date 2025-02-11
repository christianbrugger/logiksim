#ifndef LOGICSIM_CORE_VOCABULARY_UI_STATUS_H
#define LOGICSIM_CORE_VOCABULARY_UI_STATUS_H

#include "core/format/struct.h"

namespace logicsim {

/**
 * @brief: Status returned by UI functions.
 */
struct UIStatus {
    bool require_repaint {false};
    bool config_changed {false};
    bool history_changed {false};  // ???
    bool dialogs_changed {false};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const UIStatus&) const -> bool = default;
};

[[nodiscard]] constexpr auto operator|(UIStatus a, UIStatus b) noexcept -> UIStatus;
constexpr auto operator|=(UIStatus& a, UIStatus b) noexcept -> UIStatus&;

//
// Implementation
//

constexpr auto operator|(UIStatus a, UIStatus b) noexcept -> UIStatus {
    return UIStatus {
        .require_repaint = a.require_repaint || b.require_repaint,
        .config_changed = a.config_changed || b.config_changed,
        .history_changed = a.history_changed || b.history_changed,
        .dialogs_changed = a.dialogs_changed || b.dialogs_changed,
    };
};

constexpr auto operator|=(UIStatus& a, UIStatus b) noexcept -> UIStatus& {
    a = a | b;
    return a;
};

}  // namespace logicsim

#endif
