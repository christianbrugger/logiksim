#ifndef LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_SELECTION_GUARD_H
#define LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_SELECTION_GUARD_H

#include "core/selection.h"
#include "core/vocabulary/selection_id.h"

#include <gsl/gsl>

namespace logicsim {

namespace editable_circuit {

template <typename T>
concept is_selection_store = requires(T obj) {
    { obj.create_selection() } -> std::same_as<selection_id_t>;
    { obj.create_selection(Selection {}) } -> std::same_as<selection_id_t>;
    { obj.create_selection(selection_id_t {}) } -> std::same_as<selection_id_t>;
    { obj.destroy_selection(selection_id_t {}) } -> std::same_as<void>;
};

/**
 * @brief: Create a selection throughout the lifetime of the guard in the store.
 *
 * Class-invariants:
 *   + store is never null
 *   + selection_id is never null
 */
template <is_selection_store T>
class SelectionGuardTemplate {
   public:
    explicit SelectionGuardTemplate(T& store);
    explicit SelectionGuardTemplate(T& store, Selection selection_);
    explicit SelectionGuardTemplate(T& store, selection_id_t copy_id);
    ~SelectionGuardTemplate();

    SelectionGuardTemplate(SelectionGuardTemplate&&) = delete;
    SelectionGuardTemplate(const SelectionGuardTemplate&) = delete;
    auto operator=(SelectionGuardTemplate&&) -> SelectionGuardTemplate& = delete;
    auto operator=(const SelectionGuardTemplate&) -> SelectionGuardTemplate& = delete;

    [[nodiscard]] auto selection_id() const -> selection_id_t;

   private:
    gsl::not_null<T*> store_;
    selection_id_t selection_id_;
};

//
// Implementation
//

template <is_selection_store T>
SelectionGuardTemplate<T>::SelectionGuardTemplate(T& store)
    : store_ {&store}, selection_id_ {store.create_selection()} {
    Ensures(store_);
    Ensures(selection_id_);
}

template <is_selection_store T>
SelectionGuardTemplate<T>::SelectionGuardTemplate(T& store, Selection selection_)
    : store_ {&store}, selection_id_ {store.create_selection(std::move(selection_))} {
    Ensures(store_);
    Ensures(selection_id_);
}

template <is_selection_store T>
SelectionGuardTemplate<T>::SelectionGuardTemplate(T& store, selection_id_t copy_id)
    : store_ {&store}, selection_id_ {store.create_selection(copy_id)} {
    Ensures(store_);
    Ensures(selection_id_);
}

template <is_selection_store T>
SelectionGuardTemplate<T>::~SelectionGuardTemplate() {
    Expects(store_);
    Expects(selection_id_);
    store_->destroy_selection(selection_id_);
}

template <is_selection_store T>
auto SelectionGuardTemplate<T>::selection_id() const -> selection_id_t {
    Expects(store_);
    Expects(selection_id_);
    return selection_id_;
}

}  // namespace editable_circuit

}  // namespace logicsim

#endif
