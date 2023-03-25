
#include "selection.h"

namespace logicsim {

auto Selection::add_element(element_key_t element) -> void {
    selected_elements_.insert(element);
}

auto Selection::remove_element(element_key_t element) -> void {}

auto Selection::add_segment(element_key_t element, segment_index_t segment_index,
                            segment_selection_t selection) -> void {}

auto Selection::remove_segment(element_key_t element, segment_index_t segment_index,
                               segment_selection_t selection) -> void {}

auto Selection::is_selected(element_key_t element) const -> bool {
    return false;
}

}  // namespace logicsim
