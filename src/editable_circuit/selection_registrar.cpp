
#include "editable_circuit/selection_registrar.h"

#include "editable_circuit/messages.h"
#include "editable_circuit/selection.h"
#include "exceptions.h"
#include "format.h"
#include "layout.h"

namespace logicsim {

auto selection_key_t::format() const -> std::string {
    return fmt::format("{}", value);
}

auto selection_key_t::operator++() noexcept -> selection_key_t& {
    ++value;
    return *this;
}

auto selection_key_t::operator++(int) noexcept -> selection_key_t {
    auto tmp = *this;
    operator++();
    return tmp;
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

auto SelectionRegistrar::validate(const Layout& layout) const -> void {
    for (const auto& selection : selections()) {
        selection.validate(layout);
    }
}

auto SelectionRegistrar::format() const -> std::string {
    const auto item_str = fmt_join(",\n", allocated_selections_.values());
    return fmt::format("SelectionRegistrar({})", item_str);
}

auto SelectionRegistrar::submit(editable_circuit::InfoMessage message) -> void {
    for (auto& selection : selections()) {
        selection.submit(message);
    }
}

auto SelectionRegistrar::get_handle() const -> selection_handle_t {
    const auto key = next_selection_key_++;

    auto&& [it, inserted]
        = allocated_selections_.emplace(key, std::make_unique<Selection>());

    if (!inserted) {
        throw_exception("unable to create new selection.");
    }

    Selection& selection = *(it->second.get());
    return selection_handle_t {selection, *this, key};
}

auto SelectionRegistrar::get_handle(const Selection& selection) const
    -> selection_handle_t {
    auto handle = get_handle();
    handle.value() = selection;
    return handle;
}

auto SelectionRegistrar::unregister_selection(selection_key_t selection_key) const
    -> void {
    const auto delted = allocated_selections_.erase(selection_key);

    if (!delted) {
        throw_exception("unable to delete selection that should be present.");
    }
}

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
    return registrar_->get_handle(*selection_);
}

auto selection_handle_t::format() const -> std::string {
    if (has_value()) {
        return fmt::format("selection_handle_t(selection_key = {}, {})", selection_key_,
                           value());
    }
    return fmt::format("selection_handle_t(nullptr)", value());
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