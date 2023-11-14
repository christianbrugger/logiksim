#include "component/circuit_widget/mouse_logic/mouse_drag_logic.h"

#include "geometry/scene.h"

namespace logicsim {

namespace circuit_widget {

auto MouseDragLogic::mouse_press(QPointF position) -> void {
    last_position_ = position;
}

auto MouseDragLogic::mouse_move(QPointF position, const ViewConfig& config)
    -> point_fine_t {
    if (last_position_.has_value()) {
        const auto new_offset = config.offset()                   //
                                + to_grid_fine(position, config)  //
                                - to_grid_fine(*last_position_, config);
        last_position_ = position;
        return new_offset;
    }
    return config.offset();
}

auto MouseDragLogic::mouse_release(QPointF position, const ViewConfig& config)
    -> point_fine_t {
    const auto new_offset = mouse_move(position, config);
    last_position_ = std::nullopt;
    return new_offset;
}

}  // namespace circuit_widget

}  // namespace logicsim
