#ifndef LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_SELECTION_GUARD_H
#define LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_SELECTION_GUARD_H

#include "selection.h"
#include "vocabulary/selection_id.h"

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
 * @brief:
 *
 * Class-invariants:
 *   + store is never null
 *   + selection_id is never null
 */
template <is_selection_store T>
class SelectionGuard {
   public:
    explicit SelectionGuard(T& store);
    explicit SelectionGuard(T& store, Selection selection);
    explicit SelectionGuard(T& store, selection_id_t copy_id);
    ~SelectionGuard();

    [[nodiscard]] auto selection_id() const -> selection_id_t;

   private:
    gsl::not_null<T*> store_;
    selection_id_t selection_id_;
};

//
// Implementation
//

template <is_selection_store T>
SelectionGuard<T>::SelectionGuard(T& store)
    : store_ {&store}, selection_id_ {store.create_selection()} {
    Ensures(store_);
}

template <is_selection_store T>
SelectionGuard<T>::SelectionGuard(T& store, Selection selection__)
    : store_ {&store}, selection_id_ {store.create_selection(std::move(selection__))} {
    Ensures(store_);
}

template <is_selection_store T>
SelectionGuard<T>::SelectionGuard(T& store, selection_id_t copy_id)
    : store_ {&store}, selection_id_ {store.create_selection(copy_id)} {
    Ensures(store_);
}

template <is_selection_store T>
SelectionGuard<T>::~SelectionGuard() {
    Expects(selection_id_);
    store_->destroy_selection(selection_id_);
}

template <is_selection_store T>
auto SelectionGuard<T>::selection_id() const -> selection_id_t {
    Expects(selection_id_);
    return selection_id_;
}

}  // namespace editable_circuit

}  // namespace logicsim

#endif
