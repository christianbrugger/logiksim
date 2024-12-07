#ifndef LOGICSIM_CORE_GEOMETRY_LAYOUT_GEOMETRY_H
#define LOGICSIM_CORE_GEOMETRY_LAYOUT_GEOMETRY_H

#include <optional>

namespace logicsim {

struct rect_t;
class Layout;

[[nodiscard]] auto bounding_rect(const Layout& layout) -> std::optional<rect_t>;

[[nodiscard]] auto bounding_rect_logicitems(const Layout& layout)
    -> std::optional<rect_t>;

[[nodiscard]] auto bounding_rect_decorations(const Layout& layout)
    -> std::optional<rect_t>;

[[nodiscard]] auto bounding_rect_inserted_segments(const Layout& layout)
    -> std::optional<rect_t>;

[[nodiscard]] auto bounding_rect_uninserted_segments(const Layout& layout)
    -> std::optional<rect_t>;

[[nodiscard]] auto bounding_rect_segments(const Layout& layout) -> std::optional<rect_t>;

}  // namespace logicsim

#endif
