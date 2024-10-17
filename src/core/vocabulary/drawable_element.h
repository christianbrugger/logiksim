#ifndef LOGICSIM_VOCABULARY_DRAWABLE_ELEMENT_H
#define LOGICSIM_VOCABULARY_DRAWABLE_ELEMENT_H

#include "core/format/struct.h"
#include "core/vocabulary/decoration_id.h"
#include "core/vocabulary/element_draw_state.h"
#include "core/vocabulary/logicitem_id.h"

#include <string>
#include <type_traits>

namespace logicsim {

/**
 * @brief: All information needed to draw a layout logic item.
 */
struct DrawableLogicItem {
    logicitem_id_t logicitem_id;
    ElementDrawState state;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const DrawableLogicItem& other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const DrawableLogicItem& other) const = default;
};

static_assert(sizeof(DrawableLogicItem) == 8);
static_assert(std::is_aggregate_v<DrawableLogicItem>);

/**
 * @brief: All information needed to draw a layout decoration.
 */
struct DrawableDecoration {
    decoration_id_t decoration_id;
    ElementDrawState state;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const DrawableDecoration& other) const -> bool =
                                                                                default;
    [[nodiscard]] auto operator<=>(const DrawableDecoration& other) const = default;
};

static_assert(sizeof(DrawableDecoration) == 8);
static_assert(std::is_aggregate_v<DrawableDecoration>);

}  // namespace logicsim

#endif
