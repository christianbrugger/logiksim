#ifndef LOGIKSIM_SELECTION_MANAGER_H
#define LOGIKSIM_SELECTION_MANAGER_H

#include "selection.h"
#include "vocabulary.h"

#include <boost/container/vector.hpp>
#include <gsl/gsl>

#include <functional>

namespace logicsim {

enum class SelectionFunction {
    toggle,
    add,
    substract,
};

class EditableCircuit;

class SelectionBuilder {
   public:
    using selection_mask_t = boost::container::vector<bool>;

    struct operation_t {
        SelectionFunction function;
        rect_fine_t rect;
    };

   public:
    [[nodiscard]] explicit SelectionBuilder(const EditableCircuit& editable_circuit);

    [[nodiscard]] auto empty() const noexcept -> bool;

    auto clear() -> void;
    auto add(SelectionFunction function, rect_fine_t rect) -> void;
    auto update_last(rect_fine_t rect) -> void;
    auto pop_last() -> void;

    [[nodiscard]] auto selection() const -> const Selection&;
    [[nodiscard]] auto create_selection_mask() const -> selection_mask_t;

    [[nodiscard]] auto all_operations_applied() const -> bool;
    auto apply_all_operations() -> void;

    auto remove_invalid_element_keys() -> void;

   private:
    gsl::not_null<const EditableCircuit*> editable_circuit_;

    Selection initial_selection_ {};
    std::vector<operation_t> operations_ {};

    mutable std::optional<Selection> cached_selection_ {};
};

}  // namespace logicsim

#endif