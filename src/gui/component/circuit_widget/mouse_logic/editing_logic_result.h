#ifndef LOGICSIM_GUI_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_EDITING_LOGIC_RESULT_H
#define LOGICSIM_GUI_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_EDITING_LOGIC_RESULT_H

#include "core/format/struct.h"
#include "core/vocabulary/decoration_id.h"

#include <optional>

namespace logicsim {

namespace circuit_widget {

struct editing_logic_result_t {
    // TODO: move to mouse logic iteself
    // TODO: fine grained update, only update when necessary
    // TODO: flicker red on update in GUI
    // the circuit was modified and a repaint is required
    bool require_update {false};

    // new decoration was inserted
    std::optional<decoration_id_t> decoration_inserted {};

    // TODO: add ruber band
    // TODO: choose correct type
    // std::optional<QRect> rubber_band {};

    [[nodiscard]] auto operator==(const editing_logic_result_t&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::regular<editing_logic_result_t>);

}  // namespace circuit_widget

}  // namespace logicsim

#endif
