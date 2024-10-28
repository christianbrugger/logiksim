#include "gui/component/circuit_widget/mouse_logic/editing_logic_result.h"

#include "core/format/std_type.h"

#include <fmt/core.h>

namespace logicsim {

namespace circuit_widget {

auto editing_logic_result_t::format() const -> std::string {
    return fmt::format(
        "editing_logic_result_t(require_update = {}, decoration_inserted = {})",
        require_update, decoration_inserted);
}

}  // namespace circuit_widget

}  // namespace logicsim
