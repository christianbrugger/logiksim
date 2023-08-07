#ifndef LOGIKSIM_SELECTION_MANAGER_H
#define LOGIKSIM_SELECTION_MANAGER_H

#include "editable_circuit/messages_forward.h"
#include "editable_circuit/selection.h"

#include <boost/container/vector.hpp>
#include <gsl/gsl>

namespace logicsim {

enum class SelectionFunction {
    toggle,
    add,
    substract,
};

class Layout;
class SpatialTree;
class CacheProvider;

class SelectionBuilder {
   public:
    struct operation_t {
        SelectionFunction function;
        rect_fine_t rect;
    };

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

    auto submit(editable_circuit::InfoMessage message) -> void;
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