#ifndef LOGIKSIM_RENDER_WIDGET_BASE_H
#define LOGIKSIM_RENDER_WIDGET_BASE_H

#include "format/enum.h"
#include "vocabulary/widget_interaction_state.h"

#include <QWidget>

#include <string>

namespace logicsim {

struct ElementDefinition;

[[nodiscard]] auto to_logic_item_definition(InteractionState state) -> ElementDefinition;

class RendererWidgetBase : public QWidget {
    Q_OBJECT

   public:
    using QWidget::QWidget;

    Q_SIGNAL void interaction_state_changed(InteractionState new_state);

   protected:
    auto emit_interaction_state_changed(InteractionState new_state) -> void;
};

}  // namespace logicsim

#endif
