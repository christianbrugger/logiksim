#ifndef LOGICSIM_CORE_RENDER_CIRCUIT_RENDER_CONNECTOR_H
#define LOGICSIM_CORE_RENDER_CIRCUIT_RENDER_CONNECTOR_H

#include "vocabulary/element_draw_state.h"
#include "vocabulary/orientation.h"
#include "vocabulary/point.h"

namespace logicsim {

struct logicitem_id_t;
struct DrawableLogicItem;
class Layout;
class SpatialSimulation;

struct Context;

// TODO: format & equality
struct ConnectorAttributes {
    ElementDrawState state;
    point_t position;
    orientation_t orientation;
    bool is_inverted;
    bool is_enabled;
};

auto draw_connector(Context& ctx, ConnectorAttributes attributes) -> void;

auto draw_logicitem_connectors(Context& ctx, const Layout& layout,
                               logicitem_id_t logicitem_id, ElementDrawState state)
    -> void;

auto draw_logicitem_connectors(Context& ctx, const SpatialSimulation& spatial_simulation,
                               logicitem_id_t logicitem_id) -> void;

auto draw_logicitems_connectors(Context& ctx, const Layout& layout,
                                std::span<const DrawableLogicItem> elements) -> void;

auto draw_logicitems_connectors(Context& ctx, const SpatialSimulation& spatial_simulation,
                                std::span<const logicitem_id_t> elements) -> void;

}  // namespace logicsim

#endif
