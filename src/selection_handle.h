#ifndef LOGIKSIM_SELECTION_HANDLE_H
#define LOGIKSIM_SELECTION_HANDLE_H

#include "selection.h"
#include "vocabulary.h"

#include <type_traits>
#include <utility>

namespace logicsim {

class EditableCircuit;

// TODO make Selection part of the handle, so we don't introduce allocations
class selection_handle_t {
   public:
    using type = Selection;
    using reference = Selection&;
    using pointer = Selection*;

    selection_handle_t() = default;
    [[nodiscard]] explicit selection_handle_t(Selection& selection,
                                              const EditableCircuit& editable_circuit,
                                              selection_key_t selection_key);
    ~selection_handle_t();
    // disallow copying
    selection_handle_t(const selection_handle_t& other) = delete;
    auto operator=(const selection_handle_t& other) = delete;
    // allow move
    selection_handle_t(selection_handle_t&& other) noexcept;
    auto operator=(selection_handle_t&& other) noexcept -> selection_handle_t&;

    // we allow explicit copy, as it is expensive
    auto copy() -> selection_handle_t;

    [[nodiscard]] auto has_value() const noexcept -> bool;
    [[nodiscard]] operator bool() const noexcept;

    [[nodiscard]] auto value() const -> reference;
    [[nodiscard]] auto operator*() const noexcept -> reference;
    [[nodiscard]] auto get() const -> pointer;
    [[nodiscard]] auto operator->() const noexcept -> pointer;

    auto reset() noexcept -> void;
    auto swap(selection_handle_t& other) noexcept -> void;

    auto operator==(std::nullptr_t) const noexcept -> bool;

   private:
    Selection* selection_ {nullptr};
    const EditableCircuit* editable_circuit_ {nullptr};
    selection_key_t selection_key_ {null_selection_key};
};

auto swap(selection_handle_t& a, selection_handle_t& b) noexcept -> void;

}  // namespace logicsim

template <>
auto std::swap(logicsim::selection_handle_t& a, logicsim::selection_handle_t& b) noexcept
    -> void;

namespace logicsim {

static_assert(!std::is_copy_constructible_v<selection_handle_t>);
static_assert(!std::is_copy_assignable_v<selection_handle_t>);
static_assert(std::is_move_constructible_v<selection_handle_t>);
static_assert(std::is_move_assignable_v<selection_handle_t>);

// Keeps track of a single element
class element_handle_t {
   public:
    element_handle_t() = default;
    explicit element_handle_t(selection_handle_t selection_handle);

    auto clear_element() -> void;
    auto set_element(element_id_t element_id) -> void;
    auto element() const -> element_id_t;

    [[nodiscard]] operator bool() const noexcept;

   private:
    selection_handle_t selection_handle_ {};
};

}  // namespace logicsim

#endif
