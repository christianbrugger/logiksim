#ifndef LOGICSIM_CORE_COMPONENT_CIRCUIT_UI_MODEL_MOUSE_LOGIC_MOUSE_LOGIC_STATUS_H
#define LOGICSIM_CORE_COMPONENT_CIRCUIT_UI_MODEL_MOUSE_LOGIC_MOUSE_LOGIC_STATUS_H

#include "core/format/struct.h"
#include "core/vocabulary/decoration_id.h"

namespace logicsim {

struct UIStatus;

namespace circuit_ui_model {

/**
 * @brief: Generic mouse logic result returned by any mouse event logic.
 */
struct mouse_logic_status_t {
    // TODO: move to mouse logic iteself
    // TODO: fine grained update, only update when necessary
    // TODO: flicker red on update in GUI
    // the circuit was modified and a repaint is required
    bool require_repaint {false};
    bool dialogs_changed {false};

    [[nodiscard]] auto operator==(const mouse_logic_status_t &) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::regular<mouse_logic_status_t>);

[[nodiscard]] auto operator|(mouse_logic_status_t lhs,
                             mouse_logic_status_t rhs) -> mouse_logic_status_t;
auto operator|=(mouse_logic_status_t &lhs,
                mouse_logic_status_t rhs) -> mouse_logic_status_t &;

auto operator|=(UIStatus &lhs, const mouse_logic_status_t &rhs) -> UIStatus &;

/**
 * @brief: Result for mouse_release events with additional fields.
 */
struct mouse_release_status_t {
    // mouse logic is finished and can be finalized
    bool finished {true};

    mouse_logic_status_t mouse_logic_status {};

    [[nodiscard]] auto operator==(const mouse_release_status_t &) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::regular<mouse_release_status_t>);

}  // namespace circuit_ui_model

}  // namespace logicsim

#endif
