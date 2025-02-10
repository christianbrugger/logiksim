#include "core/component/circuit_ui_model/mouse_logic/mouse_logic_result.h"

#include <fmt/core.h>

namespace logicsim {

namespace circuit_ui_model {

auto mouse_logic_result_t::format() const -> std::string {
    return fmt::format(
        "mouse_logic_result_t(require_update = {}, inserted_decoration = {})",
        require_update, inserted_decoration);
}

auto mouse_release_result_t::format() const -> std::string {
    return fmt::format("editing_logic_result_t(finished = {}, {})", finished,
                       mouse_logic_result);
}

}  // namespace circuit_ui_model

}  // namespace logicsim
