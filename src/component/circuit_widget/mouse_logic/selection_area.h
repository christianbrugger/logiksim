#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_SELECTION_AREA_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_SELECTION_AREA_H

#include "vocabulary/point_fine.h"

#include <QPointF>
#include <QRubberBand>
#include <QWidget>

#include <optional>

namespace logicsim {

struct point_t;
struct ViewConfig;
class EditableCircuit;

namespace circuit_widget {

class SelectionAreaLogic {
   public:
    auto mouse_press(EditableCircuit& editable_circuit, QPointF position,
                     const ViewConfig& view_config, Qt::KeyboardModifiers modifiers)
        -> void;
    auto mouse_move(EditableCircuit& editable_circuit, QPointF position,
                    const ViewConfig& view_config, QRubberBand& rubber_band) -> void;
    auto mouse_release(EditableCircuit& editable_circuit, QPointF position,
                       const ViewConfig& view_config, QRubberBand& rubber_band) -> void;

    auto finalize(EditableCircuit& editable_circuit, QRubberBand& rubber_band) -> void;

   private:
    auto update_mouse_position(EditableCircuit& editable_circuit, QPointF position,
                               const ViewConfig& view_config, QRubberBand& rubber_band)
        -> void;

   private:
    std::optional<point_fine_t> first_position_ {};
    bool keep_last_selection_ {false};
};

}  // namespace circuit_widget

}  // namespace logicsim

#endif
