
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

selection_handle_t::~selection_handle_t() {
    if (editable_circuit_ != nullptr) {
        editable_circuit_->delete_selection(selection_key_);
    }
}

selection_handle_t::selection_handle_t(selection_handle_t&& other) noexcept {
    swap(other);
}

auto selection_handle_t::operator=(selection_handle_t&& other) noexcept
    -> selection_handle_t& {
    // we add a 'copy' so our state is destroyed other at the end of this scope
    auto temp = selection_handle_t {std::move(other)};
    swap(temp);
    return *this;
}

auto selection_handle_t::copy() -> selection_handle_t {
    if (editable_circuit_ == nullptr || selection_ == nullptr) {
        return selection_handle_t {};
    }
    return editable_circuit_->create_selection(*selection_);
}

auto selection_handle_t::reset() noexcept -> void {
    *this = selection_handle_t {};
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

auto selection_handle_t::operator==(std::nullptr_t) const noexcept -> bool {
    return selection_ == nullptr;
}

selection_handle_t::operator bool() const noexcept {
    return has_value();
}

auto selection_handle_t::has_value() const noexcept -> bool {
    return selection_ != nullptr;
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

//
// Element Handle
//

namespace logicsim {
element_handle_t::element_handle_t(selection_handle_t selection_handle)
    : selection_handle_ {std::move(selection_handle)} {
    if (!selection_handle_) [[unlikely]] {
        throw_exception("handle cannot be empty");
    }
    selection_handle_->clear();
}

auto element_handle_t::clear_element() -> void {
    if (!selection_handle_) [[unlikely]] {
        throw_exception("handle cannot be empty");
    }
    selection_handle_->clear();
}

auto element_handle_t::set_element(element_id_t element_id) -> void {
    if (!selection_handle_) [[unlikely]] {
        throw_exception("handle cannot be empty");
    }
    if (!element_id) [[unlikely]] {
        throw_exception("element_id needs to be valid.");
    }

    selection_handle_->clear();
    selection_handle_->add_element(element_id);
}

auto element_handle_t::element() const -> element_id_t {
    if (!selection_handle_) [[unlikely]] {
        throw_exception("access to empty handle");
    }

    const auto& elements = selection_handle_->selected_elements();
    const auto count = selection_handle_->selected_elements().size();

    if (count == 0) {
        return null_element;
    } else if (count == 1) {
        return elements[0];
    }
    throw_exception("selection should never have more than one element");
}

element_handle_t::operator bool() const noexcept {
    return selection_handle_ && !selection_handle_->empty();
}

}  // namespace logicsim