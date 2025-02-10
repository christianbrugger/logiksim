#ifndef LOGICSIM_CORE_COMPONENT_CIRCUIT_UI_MODEL_MOUSE_LOGIC_MOUSE_LOGIC_RESULT_H
#define LOGICSIM_CORE_COMPONENT_CIRCUIT_UI_MODEL_MOUSE_LOGIC_MOUSE_LOGIC_RESULT_H

#include "core/format/struct.h"
#include "core/vocabulary/decoration_id.h"

namespace logicsim {

namespace circuit_ui_model {

/**
 * @brief: Generic mouse logic result returned by any mouse event logic.
 */
struct mouse_logic_result_t {
    // TODO: move to mouse logic iteself
    // TODO: fine grained update, only update when necessary
    // TODO: flicker red on update in GUI
    // the circuit was modified and a repaint is required
    bool require_update {false};

    // new decoration was inserted
    decoration_id_t inserted_decoration {null_decoration_id};

    // TODO: add ruber band
    // TODO: choose correct type
    // std::optional<QRect> rubber_band {};

    [[nodiscard]] auto operator==(const mouse_logic_result_t&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::regular<mouse_logic_result_t>);

/**
 * @brief: Result for mouse_release events with additional fields.
 */
struct mouse_release_result_t {
    // mouse logic is finished and can be finalized
    bool finished {true};

    mouse_logic_result_t mouse_logic_result {};

    [[nodiscard]] auto operator==(const mouse_release_result_t&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::regular<mouse_release_result_t>);

}  // namespace circuit_ui_model

}  // namespace logicsim

#endif
