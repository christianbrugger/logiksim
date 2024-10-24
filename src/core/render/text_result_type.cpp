#include "core/render/text_result_type.h"

#include "core/format/blend2d_type.h"

#include <fmt/core.h>

namespace logicsim {

auto draw_text_result_t::format() const -> std::string {
    return fmt::format("draw_text_result_t(truncated = {}, bounding_box = {})", truncated,
                       bounding_box);
}

}  // namespace logicsim
