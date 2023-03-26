#ifndef LOGIKSIM_SELECTION_MANAGER_H
#define LOGIKSIM_SELECTION_MANAGER_H

#include "editable_circuit.h"
#include "selection.h"
#include "vocabulary.h"

#include <boost/container/vector.hpp>

namespace logicsim {

enum class SelectionFunction {
    toggle,
    add,
    substract,
};

// TODO make EditableCircuit part of constructor
class SelectionBuilder {
   public:
    using selection_mask_t = boost::container::vector<bool>;

    struct operation_t {
        SelectionFunction function;
        rect_fine_t rect;
    };

   public:
    [[nodiscard]] explicit SelectionBuilder(const EditableCircuit& editable_circuit);

    auto clear() -> void;
    auto add(SelectionFunction function, rect_fine_t rect) -> void;
    auto update_last(rect_fine_t rect) -> void;
    auto pop_last() -> void;

    // currently very slow implementation
    [[nodiscard]] auto claculate_is_item_selected(element_key_t element_key) const
        -> bool;

    [[nodiscard]] auto calculate_selection() const -> Selection;
    [[nodiscard]] auto create_selection_mask() const -> selection_mask_t;
    [[nodiscard]] auto calculate_selected_keys() const -> std::vector<element_key_t>;

    auto set_selection(Selection&& selection) -> void;
    auto bake_selection() -> void;
    [[nodiscard]] auto get_baked_selection() const -> const Selection&;

   private:
    const EditableCircuit& editable_circuit_;

    Selection initial_selection_ {};
    std::vector<operation_t> operations_ {};
};

}  // namespace logicsim

#endif