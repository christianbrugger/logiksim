#ifndef LOGICSIM_VOCABULARY_DRAWABLE_ELEMENT_H
#define LOGICSIM_VOCABULARY_DRAWABLE_ELEMENT_H

#include "format/struct.h"
#include "vocabulary/element_draw_state.h"
#include "vocabulary/element_id.h"

#include <string>

namespace logicsim {

/**
 * @brief: All information needed to draw a layout element.
 */
struct DrawableElement {
    element_id_t element_id;
    ElementDrawState state;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const DrawableElement& other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const DrawableElement& other) const = default;
};

static_assert(sizeof(DrawableElement) == 8);

}  // namespace logicsim

#endif
