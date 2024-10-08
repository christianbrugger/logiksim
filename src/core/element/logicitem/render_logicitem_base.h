#ifndef LOGICSIM_CORE_LOGIC_ITEM_RENDER_RENDER_LOGICITEM_BASE_H
#define LOGICSIM_CORE_LOGIC_ITEM_RENDER_RENDER_LOGICITEM_BASE_H

#include "vocabulary/element_draw_state.h"

#include <span>

namespace logicsim {

struct logicitem_id_t;
struct DrawableElement;
class Layout;
class SpatialSimulation;

struct Context;

auto draw_logicitem_base(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                          ElementDrawState state) -> void;

auto draw_logicitems_base(Context& ctx, const Layout& layout,
                           std::span<const DrawableElement> elements) -> void;

auto draw_logicitem_base(Context& ctx, const SpatialSimulation& spatial_simulation,
                          logicitem_id_t logicitem_id) -> void;

auto draw_logicitems_base(Context& ctx, const SpatialSimulation& spatial_simulation,
                           std::span<const logicitem_id_t> elements) -> void;

}  // namespace logicsim

#endif
