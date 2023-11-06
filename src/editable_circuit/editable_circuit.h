#ifndef LOGIKSIM_EDITABLE_CIRCUIT_H
#define LOGIKSIM_EDITABLE_CIRCUIT_H

#include "editable_circuit/cache.h"
#include "editable_circuit/message_sender.h"
#include "editable_circuit/selection_builder.h"
#include "editable_circuit/selection_registrar.h"
#include "format/struct.h"
#include "layout.h"
#include "vocabulary/line_insertion_type.h"
#include "vocabulary/logicitem_id.h"

#include <optional>

namespace logicsim {

class SelectionRegistrar;

namespace editable_circuit {
class MessageSender;
struct State;
}  // namespace editable_circuit

class EditableCircuit {
   public:
    [[nodiscard]] explicit EditableCircuit(Layout&& layout);
    [[nodiscard]] auto format() const -> std::string;
    auto validate() -> void;

    [[nodiscard]] auto layout() const -> const Layout&;
    [[nodiscard]] auto extract_layout() -> Layout;

    // adding
    auto add_example() -> void;

    auto add_logic_item(ElementDefinition definition, point_t position,
                        InsertionMode insertion_mode) -> selection_handle_t;
    auto add_logic_item(ElementDefinition definition, point_t position,
                        InsertionMode insertion_mode, const selection_handle_t& handle)
        -> void;

    auto add_line_segment(line_t line, InsertionMode insertion_mode)
        -> selection_handle_t;
    auto add_line_segment(line_t line, InsertionMode insertion_mode,
                          const selection_handle_t& handle) -> void;

    auto add_line_segments(point_t p0, point_t p1, LineInsertionType segment_type,
                           InsertionMode insertion_mode) -> selection_handle_t;
    auto add_line_segments(point_t p0, point_t p1, LineInsertionType segment_type,
                           InsertionMode insertion_mode, const selection_handle_t& handle)
        -> void;

    // changing
    auto change_insertion_mode(selection_handle_t handle,
                               InsertionMode new_insertion_mode) -> void;
    [[nodiscard]] auto new_positions_representable(const Selection& selection,
                                                   int delta_x, int delta_y) const
        -> bool;
    auto move_or_delete(selection_handle_t handle, int delta_x, int delta_y) -> void;
    // Assumptions:
    //   * all selected items are temporary
    //   * all new positions are representable
    //   * segment lines are all fully selected, not partial
    auto move_unchecked(const Selection& selection, int delta_x, int delta_y) -> void;
    auto delete_all(selection_handle_t selection) -> void;

    auto toggle_inverter(point_t point) -> void;

    auto toggle_wire_crosspoint(point_t point) -> void;

    auto set_attributes(logicitem_id_t logicitem_id, attributes_clock_generator_t attrs)
        -> void;

    // Wire Mode Change Helpers

    // adds crosspoints, merges wire segments and
    // returns splitpoints
    auto regularize_temporary_selection(
        const Selection& selection,
        std::optional<std::vector<point_t>> true_cross_points = {})
        -> std::vector<point_t>;

    auto capture_inserted_cross_points(const Selection& selection) const
        -> std::vector<point_t>;

    auto split_before_insert(const Selection& selection) -> void;

    // selections
    [[nodiscard]] auto get_handle() const -> selection_handle_t;
    [[nodiscard]] auto get_handle(const Selection& selection) const -> selection_handle_t;
    [[nodiscard]] auto selection_builder() const noexcept -> const SelectionBuilder&;
    [[nodiscard]] auto selection_builder() noexcept -> SelectionBuilder&;

    [[nodiscard]] auto caches() const -> const CacheProvider&;

   private:
    auto submit(const editable_circuit::InfoMessage& message) -> void;
    auto get_sender() -> editable_circuit::MessageSender&;
    auto get_state() -> editable_circuit::State;

    std::optional<Layout> layout_ {};

    CacheProvider cache_provider_;
    SelectionRegistrar registrar_ {};

    SelectionBuilder selection_builder_;
    editable_circuit::MessageSender sender_;
};

auto move_or_delete_points(std::span<const point_t> points, int delta_x, int delta_y)
    -> std::vector<point_t>;

}  // namespace logicsim

#endif
