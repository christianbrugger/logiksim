#ifndef LOGICSIM_CORE_COMPONENT_EDITABLE_CIRCUIT_HISTORY_STACK_H
#define LOGICSIM_CORE_COMPONENT_EDITABLE_CIRCUIT_HISTORY_STACK_H

#include "core/component/editable_circuit/visible_selection.h"
#include "core/format/enum.h"
#include "core/format/struct.h"
#include "core/stable_selection.h"
#include "core/vocabulary/decoration_key.h"
#include "core/vocabulary/endpoints.h"
#include "core/vocabulary/logicitem_key.h"
#include "core/vocabulary/move_delta.h"
#include "core/vocabulary/ordered_line.h"
#include "core/vocabulary/placed_decoration.h"
#include "core/vocabulary/placed_logicitem.h"
#include "core/vocabulary/segment_key.h"

#include <utility>
#include <vector>

namespace logicsim {

namespace editable_circuit {

enum class HistoryEntry : uint8_t {
    new_group,

    // logicitem
    logicitem_create_temporary,
    logicitem_delete_temporary,
    logicitem_move_temporary,
    logicitem_to_mode_temporary,
    logicitem_to_mode_colliding_expect_valid,
    logicitem_to_mode_colliding_assume_colliding,
    logicitem_to_mode_insert,
    logicitem_change_attributes,
    logicitem_add_visible_selection,
    logicitem_remove_visible_selection,

    // decoration
    decoration_create_temporary,
    decoration_delete_temporary,
    decoration_move_temporary,
    decoration_to_mode_temporary,
    decoration_to_mode_colliding_expect_valid,
    decoration_to_mode_colliding_assume_colliding,
    decoration_to_mode_insert,
    decoration_change_attributes,
    decoration_add_visible_selection,
    decoration_remove_visible_selection,

    // segment
    segment_create_temporary,
    segment_delete_temporary,
    segment_move_temporary,
    segment_to_mode_temporary,
    segment_to_mode_colliding_expect_valid,
    segment_to_mode_colliding_assume_colliding,
    segment_to_mode_insert,
    segment_set_endpoints,
    segment_merge,
    segment_split,
    segment_add_visible_selection,

    // visible selection
    visible_selection_clear,
    visible_selection_set,
    visible_selection_add_operation,
    visible_selection_update_last,
    visible_selection_pop_last,
    visible_selection_select_all,
};

}

template <>
[[nodiscard]] auto format(editable_circuit::HistoryEntry type) -> std::string;

namespace editable_circuit {

/*
 * @brief: Define splitting of two segments.
 *
 * Source segment is split into two segments at the offset. The additional
 * segment has new_segment as key.
 */
struct split_segment_key_t {
    segment_key_t source;
    segment_key_t new_key;
    offset_t split_offset;

    [[nodiscard]] auto operator==(const split_segment_key_t&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

/**
 * @brief: Store history actions of the editable circuit.
 *
 * Class-invariants:
 *  + All substacks have correct size to fit the entries in the main stack.
 *
 * Stack-simplifications are implemented so the following GUI events
 * don't create repeating entries
 *  + logicitem & decoration insert
 *  + logicitem & decoration resize
 *  + logicitem & decoration attribute changes
 */
class HistoryStack {
   public:
    [[nodiscard]] auto operator==(const HistoryStack&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto size() const -> std::size_t;
    auto clear() -> void;

    [[nodiscard]] auto top_entry() const -> std::optional<HistoryEntry>;

    //
    // Groups
    //

    auto push_new_group() -> bool;
    auto pop_new_group() -> void;
    [[nodiscard]] auto group_count() const -> std::size_t;

    //
    // Logicitems
    //

    auto push_logicitem_create_temporary(logicitem_key_t logicitem_key,
                                         PlacedLogicItem&& placed_logicitem) -> void;
    auto push_logicitem_delete_temporary(logicitem_key_t logicitem_key) -> void;
    auto push_logicitem_colliding_to_temporary(logicitem_key_t logicitem_key) -> void;
    auto push_logicitem_temporary_to_colliding_expect_valid(logicitem_key_t logicitem_key)
        -> void;
    auto push_logicitem_temporary_to_colliding_assume_colliding(
        logicitem_key_t logicitem_key) -> void;
    auto push_logicitem_colliding_to_insert(logicitem_key_t logicitem_key) -> void;
    auto push_logicitem_insert_to_colliding_expect_valid(logicitem_key_t logicitem_key)
        -> void;
    auto push_logicitem_move_temporary(logicitem_key_t logicitem_key,
                                       move_delta_t delta) -> void;
    auto push_logicitem_change_attributes(logicitem_key_t logicitem_key,
                                          attributes_clock_generator_t&& attrs) -> void;
    auto push_logicitem_add_visible_selection(logicitem_key_t logicitem_key) -> void;
    auto push_logicitem_remove_visible_selection(logicitem_key_t logicitem_key) -> void;

    auto pop_logicitem_create_temporary() -> std::pair<logicitem_key_t, PlacedLogicItem>;
    auto pop_logicitem_delete_temporary() -> logicitem_key_t;
    auto pop_logicitem_to_mode_temporary() -> logicitem_key_t;
    auto pop_logicitem_to_mode_colliding_expect_valid() -> logicitem_key_t;
    auto pop_logicitem_to_mode_colliding_assume_colliding() -> logicitem_key_t;
    auto pop_logicitem_to_mode_insert() -> logicitem_key_t;
    auto pop_logicitem_move_temporary() -> std::pair<logicitem_key_t, move_delta_t>;
    auto pop_logicitem_change_attributes()
        -> std::pair<logicitem_key_t, attributes_clock_generator_t>;
    auto pop_logicitem_add_visible_selection() -> logicitem_key_t;
    auto pop_logicitem_remove_visible_selection() -> logicitem_key_t;

    //
    // Decoration
    //

    auto push_decoration_create_temporary(decoration_key_t decoration_key,
                                          PlacedDecoration&& placed_decoration) -> void;
    auto push_decoration_delete_temporary(decoration_key_t decoration_key) -> void;
    auto push_decoration_colliding_to_temporary(decoration_key_t decoration_key) -> void;
    auto push_decoration_temporary_to_colliding_expect_valid(
        decoration_key_t decoration_key) -> void;
    auto push_decoration_temporary_to_colliding_assume_colliding(
        decoration_key_t decoration_key) -> void;
    auto push_decoration_colliding_to_insert(decoration_key_t decoration_key) -> void;
    auto push_decoration_insert_to_colliding_expect_valid(decoration_key_t decoration_key)
        -> void;
    auto push_decoration_move_temporary(decoration_key_t decoration_key,
                                        move_delta_t delta) -> void;
    auto push_decoration_change_attributes(decoration_key_t decoration_key,
                                           attributes_text_element_t&& attrs) -> void;
    auto push_decoration_add_visible_selection(decoration_key_t decoration_key) -> void;
    auto push_decoration_remove_visible_selection(decoration_key_t decoration_key)
        -> void;

    auto pop_decoration_create_temporary()
        -> std::pair<decoration_key_t, PlacedDecoration>;
    auto pop_decoration_delete_temporary() -> decoration_key_t;
    auto pop_decoration_to_mode_temporary() -> decoration_key_t;
    auto pop_decoration_to_mode_colliding_expect_valid() -> decoration_key_t;
    auto pop_decoration_to_mode_colliding_assume_colliding() -> decoration_key_t;
    auto pop_decoration_to_mode_insert() -> decoration_key_t;
    auto pop_decoration_move_temporary() -> std::pair<decoration_key_t, move_delta_t>;
    auto pop_decoration_change_attributes()
        -> std::pair<decoration_key_t, attributes_text_element_t>;
    auto pop_decoration_add_visible_selection() -> decoration_key_t;
    auto pop_decoration_remove_visible_selection() -> decoration_key_t;

    //
    // Segment
    //

    auto push_segment_create_temporary(segment_key_t segment_key,
                                       ordered_line_t line) -> void;
    auto push_segment_delete_temporary(segment_key_t segment_key) -> void;
    auto push_segment_move_temporary(segment_key_t segment_key,
                                     move_delta_t delta) -> void;
    auto push_segment_colliding_to_temporary(segment_key_t segment_key,
                                             part_t part) -> void;
    auto push_segment_temporary_to_colliding_expect_valid(segment_key_t segment_key,
                                                          part_t part) -> void;
    auto push_segment_temporary_to_colliding_assume_colliding(segment_key_t segment_key,
                                                              part_t part) -> void;
    auto push_segment_colliding_to_insert(segment_key_t segment_key, part_t part) -> void;
    auto push_segment_insert_to_colliding_expect_valid(segment_key_t segment_key,
                                                       part_t part) -> void;
    auto push_segment_set_endpoints(segment_key_t segment_key,
                                    endpoints_t endpoints) -> void;
    auto push_segment_merge(segment_key_t segment_key_0,
                            segment_key_t segment_key_1) -> void;
    auto push_segment_split(split_segment_key_t definition) -> void;
    auto push_segment_add_visible_selection(segment_key_t segment_key,
                                            part_t part) -> void;

    auto pop_segment_create_temporary() -> std::pair<segment_key_t, ordered_line_t>;
    auto pop_segment_delete_temporary() -> segment_key_t;
    auto pop_segment_move_temporary() -> std::pair<segment_key_t, move_delta_t>;
    auto pop_segment_to_mode_temporary() -> std::pair<segment_key_t, part_t>;
    auto pop_segment_to_mode_colliding_expect_valid() -> std::pair<segment_key_t, part_t>;
    auto pop_segment_to_mode_colliding_assume_colliding()
        -> std::pair<segment_key_t, part_t>;
    auto pop_segment_to_mode_insert() -> std::pair<segment_key_t, part_t>;
    auto pop_segment_set_endpoints() -> std::pair<segment_key_t, endpoints_t>;
    auto pop_segment_merge() -> std::pair<segment_key_t, segment_key_t>;
    auto pop_segment_split() -> split_segment_key_t;
    auto pop_segment_add_visible_selection() -> std::pair<segment_key_t, part_t>;

    //
    // Visible Selection
    //

    auto push_visible_selection_clear() -> void;
    auto push_visible_selection_set(StableSelection&& stable_selection) -> void;
    auto push_visible_selection_add_operation(
        const VisibleSelection::operation_t& operation) -> void;
    auto push_visible_selection_update_last(const rect_fine_t& rect) -> void;
    auto push_visible_selection_pop_last() -> void;
    auto push_visible_selection_select_all() -> void;

    auto pop_visible_selection_clear() -> void;
    auto pop_visible_selection_set() -> StableSelection;
    auto pop_visible_selection_add_operation() -> visible_selection::operation_t;
    auto pop_visible_selection_update_last() -> rect_fine_t;
    auto pop_visible_selection_pop_last() -> void;
    auto pop_visible_selection_select_all() -> void;

   private:
    std::size_t group_count_ {};

    // general
    std::vector<HistoryEntry> entries_ {};
    std::vector<move_delta_t> move_deltas_ {};

    // logicitem
    std::vector<logicitem_key_t> logicitem_keys_ {};
    std::vector<PlacedLogicItem> placed_logicitems_ {};

    // decoration
    std::vector<decoration_key_t> decoration_keys_ {};
    std::vector<PlacedDecoration> placed_decorations_ {};

    // segment
    std::vector<segment_key_t> segment_keys_ {};
    std::vector<ordered_line_t> lines_ {};
    std::vector<endpoints_t> endpoints_ {};
    std::vector<part_t> parts_ {};
    std::vector<offset_t> offsets_ {};

    // visible selection
    std::vector<StableSelection> selections_ {};
    std::vector<rect_fine_t> selection_rects_ {};
    std::vector<SelectionFunction> selection_functions_ {};
};

static_assert(std::regular<HistoryStack>);

//
// Free Functions
//

[[nodiscard]] auto get_entry_before_skip(const std::vector<HistoryEntry>& entries,
                                         HistoryEntry skip_type)
    -> std::optional<HistoryEntry>;

[[nodiscard]] auto last_non_group_entry(const std::vector<HistoryEntry>& entries)
    -> std::optional<HistoryEntry>;

[[nodiscard]] auto has_ungrouped_entries(const HistoryStack& stack) -> bool;

auto reopen_group(HistoryStack& stack) -> void;

}  // namespace editable_circuit

}  // namespace logicsim

#endif
