#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_SELECTION_AREA_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_SELECTION_AREA_H

#include "component/circuit_widget/mouse_logic/editing_logic_interface.h"
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

class SelectionAreaLogic : public EditingLogicInterface {
   public:
    SelectionAreaLogic(QWidget& parent);

    auto mouse_press(EditableCircuit& editable_circuit, QPointF position,
                     const ViewConfig& view_config, Qt::KeyboardModifiers modifiers)
        -> void;
    auto mouse_move(EditableCircuit& editable_circuit, QPointF position,
                    const ViewConfig& view_config) -> void;
    auto mouse_release(EditableCircuit& editable_circuit, QPointF position,
                       const ViewConfig& view_config) -> void;

    auto finalize(EditableCircuit& editable_circuit) -> void override;

   private:
    auto update_mouse_position(EditableCircuit& editable_circuit, QPointF position,
                               const ViewConfig& view_config) -> void;

   private:
    QRubberBand* rubber_band_;

    std::optional<point_fine_t> first_position_ {};
    bool keep_last_selection_ {false};
};

}  // namespace circuit_widget

}  // namespace logicsim

#endif
