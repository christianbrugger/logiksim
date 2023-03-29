
#include "selection_handle.h"

#include "editable_circuit.h"
#include "exceptions.h"

namespace logicsim {

//
// Selection Handle
//

selection_handle_t::selection_handle_t(Selection& selection,
                                       const EditableCircuit& editable_circuit,
                                       selection_key_t selection_key)
    : selection_ {&selection},
      editable_circuit_ {&editable_circuit},
      selection_key_ {selection_key} {}

auto selection_handle_t::swap(selection_handle_t& other) noexcept -> void {
    using std::swap;

    swap(selection_, other.selection_);
    swap(editable_circuit_, other.editable_circuit_);
    swap(selection_key_, other.selection_key_);
}

auto selection_handle_t::operator==(std::nullptr_t) const noexcept -> bool {
    return selection_ == nullptr;
}

auto selection_handle_t::reset() noexcept -> void {
    if (editable_circuit_ == nullptr) [[unlikely]] {
        return;
    }
    editable_circuit_->delete_selection(selection_key_);

    editable_circuit_ = nullptr;
    selection_ = nullptr;
    selection_key_ = null_selection_key;
}

selection_handle_t::~selection_handle_t() {
    reset();
}

selection_handle_t::selection_handle_t(selection_handle_t&& other) noexcept {
    swap(other);
}

auto selection_handle_t::operator=(selection_handle_t&& other) noexcept
    -> selection_handle_t& {
    // we add a 'copy' so our state is destroyed at the end of this scope
    auto copy = selection_handle_t {std::move(other)};
    swap(copy);
    return *this;
}

auto selection_handle_t::value() const -> reference {
    if (!has_value()) [[unlikely]] {
        throw_exception("selection is not set");
    }
    return *selection_;
}

auto selection_handle_t::operator*() const noexcept -> reference {
    return *selection_;
}

auto selection_handle_t::get() const -> pointer {
    return selection_;
}

auto selection_handle_t::operator->() const noexcept -> pointer {
    return selection_;
}

auto swap(selection_handle_t& a, selection_handle_t& b) noexcept -> void {
    a.swap(b);
}

}  // namespace logicsim

template <>
auto std::swap(logicsim::selection_handle_t& a, logicsim::selection_handle_t& b) noexcept
    -> void {
    a.swap(b);
}

namespace logicsim {

selection_handle_t::operator bool() const noexcept {
    return has_value();
}

auto selection_handle_t::has_value() const noexcept -> bool {
    return selection_ != nullptr;
}

}  // namespace logicsim
