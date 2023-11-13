#include "selection_resource.h"

#include <gsl/gsl>

namespace logicsim {

//

SelectionResource::SelectionResource(
    std::shared_ptr<selection_registry::ControlObject>&& control_object__)
    : control_object_ {std::move(control_object__)} {}

SelectionResource::~SelectionResource() {
    clear();
}

SelectionResource::SelectionResource(SelectionResource&& other) noexcept {
    this->swap(other);
}

auto SelectionResource::operator=(SelectionResource&& other) noexcept
    -> SelectionResource& {
    auto temp = SelectionResource {std::move(other)};
    swap(temp);
    return *this;
}

auto SelectionResource::swap(SelectionResource& other) noexcept -> void {
    using std::swap;
    swap(control_object_, other.control_object_);
}

SelectionResource::operator bool() const {
    return control_object_ && control_object_->holds_selection();
}

auto SelectionResource::clear() -> void {
    if (control_object_) {
        control_object_->clear();
    }
    control_object_.reset();
}

auto SelectionResource::selection_id() const -> selection_id_t {
    return control_object_ ? control_object_->selection_id() : null_selection_id;
}

auto swap(SelectionResource& a, SelectionResource& b) noexcept -> void {
    a.swap(b);
}

}  // namespace logicsim
