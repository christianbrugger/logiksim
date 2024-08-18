#ifndef LOGICSIM_RENDER_PRIMITIVE_STROKE_H
#define LOGICSIM_RENDER_PRIMITIVE_STROKE_H

namespace logicsim {

struct ViewConfig;
struct Context;

namespace defaults {
constexpr inline static auto use_view_config_stroke_width = int {-1};
}  // namespace defaults

auto resolve_stroke_width(int attribute, const ViewConfig& view_config) -> int;
auto resolve_stroke_width(int attribute, const Context& ctx) -> int;

auto stroke_offset(int stroke_width) -> double;

}  // namespace logicsim

#endif
