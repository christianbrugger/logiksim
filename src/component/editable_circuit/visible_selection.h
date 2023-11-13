#ifndef LOGIKSIM_COMPONENT_EDITABLE_CIRCUIT_VISIBLE_SELECTION_H
#define LOGIKSIM_COMPONENT_EDITABLE_CIRCUIT_VISIBLE_SELECTION_H

#include "layout_message.h"
#include "selection.h"
#include "format/enum.h"
#include "vocabulary/rect_fine.h"

#include <gsl/gsl>

namespace logicsim {

class Layout;
class SelectionIndex;
class LayoutIndex;

enum class SelectionFunction {
    toggle,
    add,
    substract,
};

template <>
auto format(SelectionFunction selection_function) -> std::string;

namespace selection_builder {

struct operation_t {
    SelectionFunction function;
    rect_fine_t rect;
};

}  // namespace selection_builder

// TODO rename to Selection
class SelectionBuilder {
   public:
    using operation_t = selection_builder::operation_t;

   public:
    [[nodiscard]] explicit SelectionBuilder(const Layout &layout,
                                            const LayoutIndex &cache_provider);

    [[nodiscard]] auto empty() const noexcept -> bool;

    auto clear() -> void;
    auto add(SelectionFunction function, rect_fine_t rect) -> void;
    // TODO remove
    auto update_last(rect_fine_t rect) -> void;
    auto pop_last() -> void;
    auto set_selection(Selection selection) -> void;

    [[nodiscard]] auto selection() const -> const Selection &;

    // TODO remove
    [[nodiscard]] auto all_operations_applied() const -> bool;
    auto apply_all_operations() -> void;

    auto submit(const editable_circuit::InfoMessage &message) -> void;
    auto validate(const Layout &layout) const -> void;

   private:
    auto calculate_selection() const -> Selection;
    auto clear_cache() const -> void;

    gsl::not_null<const Layout *> layout_;
    gsl::not_null<const LayoutIndex *> layout_index_;

    Selection initial_selection_ {};
    std::vector<operation_t> operations_ {};
    mutable std::optional<Selection> cached_selection_ {};
};

}  // namespace logicsim

#endif