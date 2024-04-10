#ifndef LOGICSIM_VOCABULARY_DRAWABLE_ELEMENT_H
#define LOGICSIM_VOCABULARY_DRAWABLE_ELEMENT_H

#include "format/struct.h"
#include "vocabulary/element_draw_state.h"
#include "vocabulary/logicitem_id.h"

#include <string>
#include <type_traits>

namespace logicsim {

/**
 * @brief: All information needed to draw a layout element.
 */
struct DrawableElement {
    logicitem_id_t logicitem_id;
    ElementDrawState state;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const DrawableElement& other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const DrawableElement& other) const = default;
};

static_assert(sizeof(DrawableElement) == 8);
static_assert(std::is_aggregate_v<DrawableElement>);

}  // namespace logicsim

#endif
