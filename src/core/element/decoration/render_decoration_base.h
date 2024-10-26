#ifndef LOGICSIM_CORE_ELEMENT_DECORATION_RENDER_DECORATION_BASE_H
#define LOGICSIM_CORE_ELEMENT_DECORATION_RENDER_DECORATION_BASE_H

#include "core/vocabulary/element_draw_state.h"

#include <span>

namespace logicsim {

struct decoration_id_t;
struct DrawableDecoration;
class Layout;

struct Context;

auto draw_decoration_base(Context& ctx, const Layout& layout,
                          decoration_id_t decoration_id, ElementDrawState state) -> void;

auto draw_decorations_base(Context& ctx, const Layout& layout,
                           std::span<const DrawableDecoration> elements) -> void;

auto draw_decorations_base(Context& ctx, const Layout& layout,
                           std::span<const decoration_id_t> elements,
                           ElementDrawState state) -> void;

}  // namespace logicsim

#endif
