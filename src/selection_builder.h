#ifndef LOGIKSIM_SELECTION_MANAGER_H
#define LOGIKSIM_SELECTION_MANAGER_H

#include "selection.h"
#include "selection_handle.h"
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
    [[nodiscard]] auto copy_selection() const -> selection_handle_t;

    [[nodiscard]] auto all_operations_applied() const -> bool;
    auto apply_all_operations() -> void;

   private:
    gsl::not_null<const EditableCircuit*> editable_circuit_;

    gsl::not_null<selection_handle_t> initial_selection_;
    std::vector<operation_t> operations_ {};

    mutable selection_handle_t cached_selection_ {};
};

}  // namespace logicsim

#endif