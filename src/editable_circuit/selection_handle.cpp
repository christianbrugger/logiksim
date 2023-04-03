
#include "editable_circuit/selection_handle.h"

#include "exceptions.h"

namespace logicsim {

//
// Selection Handle
//

selection_handle_t::selection_handle_t(Selection& selection,
                                       const SelectionRegistrar& registrar,
                                       selection_key_t selection_key)
    : selection_ {&selection}, registrar_ {&registrar}, selection_key_ {selection_key} {}

auto selection_handle_t::swap(selection_handle_t& other) noexcept -> void {
    using std::swap;

    swap(selection_, other.selection_);
    swap(registrar_, other.registrar_);
    swap(selection_key_, other.selection_key_);
}

selection_handle_t::~selection_handle_t() {
    if (registrar_ != nullptr) {
        registrar_->unregister_selection(selection_key_);
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

auto selection_handle_t::copy() const -> selection_handle_t {
    if (registrar_ == nullptr || selection_ == nullptr) {
        return selection_handle_t {};
    }
    return registrar_->create_selection(*selection_);
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

//
// Selection Registrar
//

namespace detail::selection_registrar {

auto unpack_selection(const selection_map_t::value_type& value) -> Selection& {
    if (value.second == nullptr) [[unlikely]] {
        throw_exception("selection cannot be null");
    }
    return *value.second;
}

}  // namespace detail::selection_registrar

auto SelectionRegistrar::validate(const Circuit& circuit) const -> void {
    for (const auto& item : allocated_selections_) {
        if (item.second == nullptr) [[unlikely]] {
            throw_exception("selection cannot be nullptr");
        }
        item.second->validate(circuit);
    }
}

auto SelectionRegistrar::create_selection() const -> selection_handle_t {
    const auto key = next_selection_key_++;

    auto&& [it, inserted]
        = allocated_selections_.emplace(key, std::make_unique<Selection>());

    if (!inserted) {
        throw_exception("unable to create new selection.");
    }

    Selection& selection = *(it->second.get());
    return selection_handle_t {selection, *this, key};
}

auto SelectionRegistrar::create_selection(const Selection& selection) const
    -> selection_handle_t {
    auto handle = create_selection();
    handle.value() = selection;
    return handle;
}

auto SelectionRegistrar::element_handle() const -> element_handle_t {
    return element_handle_t {create_selection()};
}

auto SelectionRegistrar::element_handle(element_id_t element_id) const
    -> element_handle_t {
    auto handle = element_handle_t {create_selection()};
    handle.set_element(element_id);
    return handle;
}

auto SelectionRegistrar::unregister_selection(selection_key_t selection_key) const
    -> void {
    if (!allocated_selections_.erase(selection_key)) {
        throw_exception("unable to delete selection that should be present.");
    }
}

}  // namespace logicsim