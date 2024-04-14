#ifndef LOGIKSIM_COMPONENT_EDITABLE_CIRCUIT_VISIBLE_SELECTION_H
#define LOGIKSIM_COMPONENT_EDITABLE_CIRCUIT_VISIBLE_SELECTION_H

#include "format/enum.h"
#include "format/struct.h"
#include "layout_message.h"
#include "selection.h"
#include "vocabulary/rect_fine.h"

#include <optional>

namespace logicsim {

class Layout;
class SpatialIndex;
class LayoutIndex;

enum class SelectionFunction {
    add,
    substract,
};

template <>
auto format(SelectionFunction selection_function) -> std::string;

namespace visible_selection {

struct operation_t {
    SelectionFunction function;
    rect_fine_t rect;

    [[nodiscard]] auto format() const -> std ::string;
    [[nodiscard]] auto operator==(const operation_t &) const -> bool = default;
};

}  // namespace visible_selection

/**
 * @brief: Stores a visible selection, areas of positive and negative rectangles.
 *
 * Class-invariant:
 *   + cached_selection_ is only set if operations are non-empty
 */
class VisibleSelection {
   public:
    using operation_t = visible_selection::operation_t;

   public:
    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    [[nodiscard]] auto format() const -> std ::string;
    [[nodiscard]] auto operator==(const VisibleSelection &other) const -> bool;

    auto clear() -> void;
    auto add(SelectionFunction function, rect_fine_t rect) -> void;
    auto update_last(rect_fine_t rect) -> void;
    auto pop_last() -> void;
    auto set_selection(Selection selection) -> void;
    [[nodiscard]] auto operation_count() const -> std::size_t;

    auto apply_all_operations(const Layout &layout, const LayoutIndex &layout_index)
        -> void;
    [[nodiscard]] auto selection(const Layout &layout,
                                 const LayoutIndex &layout_index) const
        -> const Selection &;

    auto submit(const InfoMessage &message) -> void;

   private:
    [[nodiscard]] auto calculate_selection(const Layout &layout,
                                           const LayoutIndex &layout_index) const
        -> Selection;

    [[nodiscard]] auto class_invariant_holds() const -> bool;

   private:
    Selection initial_selection_ {};

    std::vector<operation_t> operations_ {};
    mutable std::optional<Selection> cached_selection_ {};
};

static_assert(std::regular<VisibleSelection>);

}  // namespace logicsim

#endif