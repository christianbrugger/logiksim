#include "gui/component/circuit_widget/mouse_logic/selection_area.h"

#include "gui/qt/point_conversion.h"

#include "core/algorithm/sort_pair.h"
#include "core/editable_circuit.h"
#include "core/geometry/scene.h"

namespace logicsim {

namespace circuit_widget {

namespace {

auto calculate_q_rect(std::optional<point_fine_t> first_position, QPointF position,
                      const ViewConfig& view_config) -> QRect {
    if (!first_position) {
        return QRect {position.toPoint(), position.toPoint()};
    }

    // order points
    const auto q0 = from(to_device(*first_position, view_config));
    const auto q1 = position.toPoint();
    const auto [x0, x1] = sorted(q0.x(), q1.x());
    const auto [y0, y1] = sorted(q0.y(), q1.y());

    // QRect
    const auto q_minimum = QPoint {x0, y0};
    const auto q_maximum = QPoint {x1, y1};
    return QRect {q_minimum, q_maximum};
}

auto to_rect_fine(QRect qrect, const ViewConfig& view_config) -> rect_fine_t {
    const auto a_minimum = to_grid_fine(to(qrect.topLeft()), view_config);
    const auto a_maximum = to_grid_fine(to(qrect.bottomRight()), view_config);
    return rect_fine_t {a_minimum, a_maximum};
}

}  // namespace

auto SelectionAreaLogic::mouse_press(EditableCircuit& editable_circuit, QPointF position,
                                     const ViewConfig& view_config,
                                     Qt::KeyboardModifiers modifiers) -> void {
    const auto p0 = to_grid_fine(to(position), view_config);

    const auto function = [modifiers] {
        if (modifiers == Qt::AltModifier) {
            return SelectionFunction::substract;
        }
        return SelectionFunction::add;
    }();

    if (modifiers == Qt::NoModifier) {
        editable_circuit.clear_visible_selection();
    }

    editable_circuit.add_visible_selection_rect(function, rect_fine_t {p0, p0});
    first_position_ = p0;
    keep_last_selection_ = false;
}

auto SelectionAreaLogic::mouse_move(EditableCircuit& editable_circuit, QPointF position,
                                    const ViewConfig& view_config,
                                    QRubberBand& rubber_band) -> void {
    update_mouse_position(editable_circuit, position, view_config, rubber_band);
}

auto SelectionAreaLogic::mouse_release(EditableCircuit& editable_circuit,
                                       QPointF position, const ViewConfig& view_config,
                                       QRubberBand& rubber_band) -> void {
    update_mouse_position(editable_circuit, position, view_config, rubber_band);
    keep_last_selection_ = true;
}

auto SelectionAreaLogic::finalize(EditableCircuit& editable_circuit,
                                  QRubberBand& rubber_band) -> void {
    if (!keep_last_selection_) {
        editable_circuit.try_pop_last_visible_selection_rect();
    }
    editable_circuit.apply_all_visible_selection_operations();

    // reset
    first_position_.reset();
    keep_last_selection_ = false;
    rubber_band.hide();

    editable_circuit.finish_undo_group();
}

auto SelectionAreaLogic::update_mouse_position(EditableCircuit& editable_circuit,
                                               QPointF position,
                                               const ViewConfig& view_config,
                                               QRubberBand& rubber_band) -> void {
    const auto q_rect = calculate_q_rect(first_position_, position, view_config);

    rubber_band.setGeometry(q_rect);
    rubber_band.show();

    editable_circuit.try_update_last_visible_selection_rect(
        to_rect_fine(q_rect, view_config));
}

}  // namespace circuit_widget

}  // namespace logicsim
