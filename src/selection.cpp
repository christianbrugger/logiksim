
#include "selection.h"

namespace logicsim {

auto Selection::swap(Selection &other) noexcept -> void {
    using std::swap;

    selected_elements_.swap(other.selected_elements_);
    selected_segments_.swap(other.selected_segments_);
}

auto Selection::clear() -> void {
    selected_elements_.clear();
    selected_segments_.clear();
}

auto Selection::format() const -> std::string {
    return fmt::format("Slection({})", selected_elements_.values());
}

auto Selection::empty() const noexcept -> bool {
    return selected_elements_.empty() && selected_segments_.empty();
}

auto Selection::add_element(element_key_t element) -> void {
    selected_elements_.insert(element);
}

auto Selection::remove_element(element_key_t element) -> void {
    selected_elements_.erase(element);
}

auto Selection::toggle_element(element_key_t element) -> void {
    if (is_selected(element)) {
        remove_element(element);
    } else {
        add_element(element);
    }
}

auto Selection::add_segment(element_key_t element, segment_index_t segment_index,
                            segment_selection_t selection) -> void {}

auto Selection::remove_segment(element_key_t element, segment_index_t segment_index,
                               segment_selection_t selection) -> void {}

auto Selection::is_selected(element_key_t element) const -> bool {
    return selected_elements_.contains(element);
}

auto Selection::selected_elements() const -> std::span<const element_key_t> {
    return selected_elements_.values();
}

auto swap(Selection &a, Selection &b) noexcept -> void {
    a.swap(b);
}

}  // namespace logicsim

template <>
auto std::swap(logicsim::Selection &a, logicsim::Selection &b) noexcept -> void {
    a.swap(b);
}
