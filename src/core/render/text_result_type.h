#ifndef LOGICSIM_CORE_RENDER_TEXT_RESULT_TYPE_H
#define LOGICSIM_CORE_RENDER_TEXT_RESULT_TYPE_H

#include "core/format/struct.h"
#include "core/vocabulary/text_truncated.h"

#include <blend2d/blend2d.h>

namespace logicsim {

struct draw_text_result_t {
    TextTruncated truncated;
    BLBox bounding_box;

    [[nodiscard]] auto operator==(const draw_text_result_t&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

}  // namespace logicsim

#endif
