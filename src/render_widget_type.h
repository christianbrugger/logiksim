#ifndef LOGIKSIM_RENDER_WIDGET_TYPE_H
#define LOGIKSIM_RENDER_WIDGET_TYPE_H

#include "format.h"

#include <QWidget>

#include <string>

namespace logicsim {

struct LogicItemDefinition;

enum class InteractionState {
    not_interactive,
    selection,
    simulation,

    insert_wire,
    insert_button,
    insert_led,

    insert_and_element,
    insert_or_element,
    insert_xor_element,
    insert_nand_element,
    insert_nor_element,

    insert_buffer_element,
    insert_inverter_element,
    insert_flipflop_jk,
    insert_latch_d,
    insert_flipflop_d,
    insert_flipflop_ms_d,

    insert_clock_generator,
    insert_shift_register,
};

template <>
auto format(InteractionState type) -> std::string;

[[nodiscard]] auto is_inserting_state(InteractionState state) -> bool;

[[nodiscard]] auto to_logic_item_definition(InteractionState state,
                                            std::size_t default_input_count = 3)
    -> LogicItemDefinition;

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
