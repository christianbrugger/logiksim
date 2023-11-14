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
class SelectionIndex;
class LayoutIndex;

enum class SelectionFunction {
    //    toggle,
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

// TODO rename to VisibleSelection
class VisibleSelection {
   public:
    using operation_t = visible_selection::operation_t;

   public:
    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    [[nodiscard]] auto format() const -> std ::string;
    [[nodiscard]] auto operator==(const VisibleSelection &) const -> bool;

    auto clear() -> void;
    auto add(SelectionFunction function, rect_fine_t rect) -> void;
    auto update_last(rect_fine_t rect) -> void;
    auto pop_last() -> void;
    auto set_selection(Selection selection) -> void;

    auto apply_all_operations(const Layout &layout, const LayoutIndex &layout_index)
        -> void;
    [[nodiscard]] auto selection(const Layout &layout,
                                 const LayoutIndex &layout_index) const
        -> const Selection &;

    auto submit(const editable_circuit::InfoMessage &message) -> void;

    // TODO remove
    auto validate(const Layout &layout, const LayoutIndex &layout_index) const -> void;

   private:
    auto calculate_selection(const Layout &layout, const LayoutIndex &layout_index) const
        -> Selection;
    auto clear_cache() const -> void;

    Selection initial_selection_ {};

    std::vector<operation_t> operations_ {};
    mutable std::optional<Selection> cached_selection_ {};
};

static_assert(std::regular<VisibleSelection>);

}  // namespace logicsim

#endif