#ifndef LOGIKSIM_SELECTION_MANAGER_H
#define LOGIKSIM_SELECTION_MANAGER_H

#include "editable_circuit/message_forward.h"
#include "editable_circuit/selection.h"
#include "format/enum.h"
#include "vocabulary/rect_fine.h"

#include <gsl/gsl>

namespace logicsim {

class Layout;
class SpatialTree;
class CacheProvider;

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

class SelectionBuilder {
   public:
    using operation_t = selection_builder::operation_t;

   public:
    [[nodiscard]] explicit SelectionBuilder(const Layout &layout,
                                            const CacheProvider &cache_provider);

    [[nodiscard]] auto empty() const noexcept -> bool;

    auto clear() -> void;
    auto add(SelectionFunction function, rect_fine_t rect) -> void;
    auto update_last(rect_fine_t rect) -> void;
    auto pop_last() -> void;
    auto set_selection(Selection selection) -> void;

    [[nodiscard]] auto selection() const -> const Selection &;

    [[nodiscard]] auto all_operations_applied() const -> bool;
    auto apply_all_operations() -> void;

    auto submit(const editable_circuit::InfoMessage &message) -> void;
    auto validate(const Layout &layout) const -> void;

   private:
    auto calculate_selection() const -> Selection;
    auto clear_cache() const -> void;

    gsl::not_null<const Layout *> layout_;
    gsl::not_null<const CacheProvider *> cache_provider_;

    Selection initial_selection_ {};
    std::vector<operation_t> operations_ {};
    mutable std::optional<Selection> cached_selection_ {};
};

}  // namespace logicsim

#endif