#ifndef LOGIKSIM_EDITABLE_CIRCUIT_H
#define LOGIKSIM_EDITABLE_CIRCUIT_H

#include "editable_circuit/caches.h"
#include "editable_circuit/messages.h"
#include "editable_circuit/selection_builder.h"
#include "editable_circuit/selection_registrar.h"
#include "layout.h"

#include <optional>

namespace logicsim {

class selection_handle_t;

namespace editable_circuit {
class MessageSender;
struct State;
}  // namespace editable_circuit

class EditableCircuit {
   public:
    [[nodiscard]] EditableCircuit(Layout&& layout);
    [[nodiscard]] auto format() const -> std::string;
    auto validate() -> void;

    [[nodiscard]] auto layout() const -> const Layout&;
    [[nodiscard]] auto extract_layout() -> Layout;

    // adding
    auto add_example() -> void;
    auto add_logic_item(LogicItemDefinition definition, point_t position,
                        InsertionMode insertion_mode) -> selection_handle_t;

    auto add_line_segments(point_t p0, point_t p1, LineSegmentType segment_type,
                           InsertionMode insertion_mode) -> selection_handle_t;

    // changing
    auto change_insertion_mode(selection_handle_t handle,
                               InsertionMode new_insertion_mode) -> void;
    [[nodiscard]] auto new_positions_representable(const Selection& selection,
                                                   int delta_x, int delta_y) const
        -> bool;
    auto move_or_delete_elements(selection_handle_t handle, int delta_x, int delta_y)
        -> void;
    auto delete_all(selection_handle_t selection) -> void;

    // selections
    [[nodiscard]] auto create_selection() const -> selection_handle_t;
    [[nodiscard]] auto create_selection(const Selection& selection) const
        -> selection_handle_t;
    [[nodiscard]] auto selection_builder() const noexcept -> const SelectionBuilder&;
    [[nodiscard]] auto selection_builder() noexcept -> SelectionBuilder&;

    [[nodiscard]] auto caches() const -> const CacheProvider&;
    auto _submit(editable_circuit::InfoMessage message) -> void;

   private:
    auto get_sender() -> editable_circuit::MessageSender;
    auto get_state() -> editable_circuit::State;

    std::optional<Layout> layout_ {std::nullopt};

    CacheProvider cache_provider_ {};
    SelectionRegistrar registrar_ {};
    SelectionBuilder selection_builder_;
};

}  // namespace logicsim

#endif
