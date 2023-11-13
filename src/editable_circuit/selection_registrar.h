#ifndef LOGIKSIM_SELECTION_HANDLE_H
#define LOGIKSIM_SELECTION_HANDLE_H

#include "editable_circuit/message_forward.h"
#include "format/struct.h"
#include "iterator_adaptor/transform_view.h"
#include "vocabulary/selection_id.h"

#include <ankerl/unordered_dense.h>

#include <memory>
#include <type_traits>
#include <utility>

namespace logicsim {

class Layout;
class selection_old_handle_t;
class Selection;

// TODO remove detail namespace
namespace detail::selection_registrar {

// TODO don't store unique ptr
using selection_map_t =
    ankerl::unordered_dense::map<selection_id_t, std::unique_ptr<Selection>>;

// TODO remove
auto unpack_selection(const selection_map_t::value_type& value) -> Selection&;

}  // namespace detail::selection_registrar

// TODO rename to SelectionRegistry
class SelectionRegistrar {
   public:
    [[nodiscard]] auto format() const -> std::string;

    auto submit(const editable_circuit::InfoMessage& message) -> void;
    auto validate(const Layout& layout) const -> void;

    [[nodiscard]] auto get_handle() const -> selection_old_handle_t;
    [[nodiscard]] auto get_handle(const Selection& selection) const -> selection_old_handle_t;

    [[nodiscard]] auto selections() const {
        return transform_view(allocated_selections_,
                              detail::selection_registrar::unpack_selection);
    }

   private:
    friend selection_old_handle_t;
    auto unregister_selection(selection_id_t selection_key) const -> void;

    using selection_map_t = detail::selection_registrar::selection_map_t;

    // we want our state to be mutable, as we are like an allocator
    mutable selection_id_t next_selection_key_ {0};
    mutable selection_map_t allocated_selections_ {};
};

//
// Handle
//

// TODO make Selection part of the handle, so we don't introduce allocations
class selection_old_handle_t {
   public:
    using type = Selection;
    using reference = Selection&;
    using pointer = Selection*;

    selection_old_handle_t() = default;
    [[nodiscard]] explicit selection_old_handle_t(Selection& selection,
                                              const SelectionRegistrar& registrar,
                                              selection_id_t selection_key);
    ~selection_old_handle_t();
    // disallow copying
    selection_old_handle_t(const selection_old_handle_t& other) = delete;
    auto operator=(const selection_old_handle_t& other) = delete;
    // allow move
    selection_old_handle_t(selection_old_handle_t&& other) noexcept;
    auto operator=(selection_old_handle_t&& other) noexcept -> selection_old_handle_t&;

    // we allow explicit copy, as it is expensive
    auto copy() const -> selection_old_handle_t;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto has_value() const noexcept -> bool;
    [[nodiscard]] operator bool() const noexcept;

    [[nodiscard]] auto value() const -> reference;
    [[nodiscard]] auto operator*() const noexcept -> reference;
    [[nodiscard]] auto get() const -> pointer;
    [[nodiscard]] auto operator->() const noexcept -> pointer;

    auto reset() noexcept -> void;
    auto swap(selection_old_handle_t& other) noexcept -> void;

    auto operator==(std::nullptr_t) const noexcept -> bool;

   private:
    Selection* selection_ {nullptr};
    const SelectionRegistrar* registrar_ {nullptr};
    selection_id_t selection_key_ {null_selection_id};
};

auto swap(selection_old_handle_t& a, selection_old_handle_t& b) noexcept -> void;

}  // namespace logicsim

template <>
auto std::swap(logicsim::selection_old_handle_t& a, logicsim::selection_old_handle_t& b) noexcept
    -> void;

namespace logicsim {

static_assert(!std::is_copy_constructible_v<selection_old_handle_t>);
static_assert(!std::is_copy_assignable_v<selection_old_handle_t>);
static_assert(std::is_move_constructible_v<selection_old_handle_t>);
static_assert(std::is_move_assignable_v<selection_old_handle_t>);

}  // namespace logicsim

#endif
