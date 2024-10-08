#ifndef LOGICSIM_CORE_RENDER_CIRCUIT_RENDER_OVERLAY_H
#define LOGICSIM_CORE_RENDER_CIRCUIT_RENDER_OVERLAY_H

#include "format/enum.h"

#include <span>

namespace logicsim {

struct color_t;
struct ordered_line_t;

struct logicitem_id_t;
struct segment_info_t;
class Layout;

struct Context;

enum class shadow_t : uint8_t {
    selected,
    valid,
    colliding,
};

template <>
[[nodiscard]] auto format(shadow_t state) -> std::string;

auto shadow_color(shadow_t shadow_type) -> color_t;

auto draw_logicitem_shadow(Context& ctx, const Layout& layout,
                            logicitem_id_t logicitem_id, shadow_t shadow_type) -> void;

auto draw_logicitem_shadows(Context& ctx, const Layout& layout,
                             std::span<const logicitem_id_t> elements,
                             shadow_t shadow_type) -> void;

auto draw_wire_shadows(Context& ctx, std::span<const ordered_line_t> lines,
                       shadow_t shadow_type) -> void;

auto draw_wire_shadows(Context& ctx, std::span<const segment_info_t> segment_infos,
                       shadow_t shadow_type) -> void;

}  // namespace logicsim

#endif
