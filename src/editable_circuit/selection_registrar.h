#ifndef LOGIKSIM_SELECTION_HANDLE_H
#define LOGIKSIM_SELECTION_HANDLE_H

#include "editable_circuit/message_forward.h"
#include "iterator_adaptor.h"

#include <type_traits>
#include <utility>

namespace logicsim {

class Layout;
class selection_handle_t;
class Selection;

struct selection_key_t {
    using value_type = int64_t;
    value_type value;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const selection_key_t& other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const selection_key_t& other) const = default;

    auto operator++() noexcept -> selection_key_t&;
    auto operator++(int) noexcept -> selection_key_t;
};

static_assert(std::is_trivial_v<selection_key_t>);

inline constexpr auto null_selection_key = selection_key_t {-1};

}  // namespace logicsim

template <>
struct ankerl::unordered_dense::hash<logicsim::selection_key_t> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const logicsim::selection_key_t& obj) const noexcept
        -> uint64_t {
        return detail::wyhash::hash(static_cast<uint64_t>(obj.value));
    }
};

//
// Registrar
//
namespace logicsim {

namespace detail::selection_registrar {

using selection_map_t =
    ankerl::unordered_dense::map<selection_key_t, std::unique_ptr<Selection>>;

auto unpack_selection(const selection_map_t::value_type& value) -> Selection&;

}  // namespace detail::selection_registrar

class SelectionRegistrar {
   public:
    [[nodiscard]] auto format() const -> std::string;

    auto submit(const editable_circuit::InfoMessage& message) -> void;
    auto validate(const Layout& layout) const -> void;

    [[nodiscard]] auto get_handle() const -> selection_handle_t;
    [[nodiscard]] auto get_handle(const Selection& selection) const -> selection_handle_t;

    [[nodiscard]] auto selections() const {
        return transform_view(allocated_selections_,
                              detail::selection_registrar::unpack_selection);
    }

   private:
    friend selection_handle_t;
    auto unregister_selection(selection_key_t selection_key) const -> void;

    using selection_map_t = detail::selection_registrar::selection_map_t;

    // we want our state to be mutable, as we are like an allocator
    mutable selection_key_t next_selection_key_ {0};
    mutable selection_map_t allocated_selections_ {};
};

//
// Handle
//

// TODO make Selection part of the handle, so we don't introduce allocations
class selection_handle_t {
   public:
    using type = Selection;
    using reference = Selection&;
    using pointer = Selection*;

    selection_handle_t() = default;
    [[nodiscard]] explicit selection_handle_t(Selection& selection,
                                              const SelectionRegistrar& registrar,
                                              selection_key_t selection_key);
    ~selection_handle_t();
    // disallow copying
    selection_handle_t(const selection_handle_t& other) = delete;
    auto operator=(const selection_handle_t& other) = delete;
    // allow move
    selection_handle_t(selection_handle_t&& other) noexcept;
    auto operator=(selection_handle_t&& other) noexcept -> selection_handle_t&;

    // we allow explicit copy, as it is expensive
    auto copy() const -> selection_handle_t;

    [[nodiscard]] auto format() const -> std::string;

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
    const SelectionRegistrar* registrar_ {nullptr};
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

}  // namespace logicsim

#endif
