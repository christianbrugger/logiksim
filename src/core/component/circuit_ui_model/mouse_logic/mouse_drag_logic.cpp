#include "core/component/circuit_ui_model/mouse_logic/mouse_drag_logic.h"

#include "core/geometry/scene.h"

namespace logicsim {

namespace circuit_ui_model {

auto MouseDragLogic::mouse_press(point_device_fine_t position) -> void {
    last_position_ = position;
}

auto MouseDragLogic::mouse_move(point_device_fine_t position,
                                const ViewConfig& config) -> point_fine_t {
    if (last_position_.has_value()) {
        const auto new_offset = config.offset()                   //
                                + to_grid_fine(position, config)  //
                                - to_grid_fine(*last_position_, config);
        last_position_ = position;
        return new_offset;
    }
    return config.offset();
}

auto MouseDragLogic::mouse_release(point_device_fine_t position,
                                   const ViewConfig& config) -> point_fine_t {
    const auto new_offset = mouse_move(position, config);
    last_position_ = std::nullopt;
    return new_offset;
}

}  // namespace circuit_ui_model

}  // namespace logicsim
