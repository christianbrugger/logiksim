#ifndef LOGICSIM_CORE_RENDER_CIRCUIT_RENDER_WIRE_H
#define LOGICSIM_CORE_RENDER_CIRCUIT_RENDER_WIRE_H

#include "format/struct.h"
#include "vocabulary/element_draw_state.h"

#include <concepts>
#include <span>
#include <string>
#include <type_traits>

namespace logicsim {

struct color_t;
struct point_t;
struct line_fine_t;
struct ordered_line_t;
struct segment_info_t;

struct wire_id_t;
class Layout;
class SpatialSimulation;

struct Context;

auto wire_color(bool is_enabled) -> color_t;

auto wire_color(bool is_enabled, ElementDrawState state) -> color_t;

auto draw_line_cross_point(Context& ctx, point_t point, bool is_enabled,
                           ElementDrawState state) -> void;

struct SegmentAttributes {
    bool is_enabled {false};
    bool p0_endcap {false};
    bool p1_endcap {false};

    [[nodiscard]] auto operator==(const SegmentAttributes&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::is_aggregate_v<SegmentAttributes>);
static_assert(std::regular<SegmentAttributes>);

auto draw_line_segment(Context& ctx, line_fine_t line, SegmentAttributes attributes,
                       ElementDrawState state) -> void;

auto draw_line_segment(Context& ctx, ordered_line_t line, SegmentAttributes attributes,
                       ElementDrawState state) -> void;

auto draw_line_segment(Context& ctx, segment_info_t info, bool is_enabled,
                       ElementDrawState state) -> void;

auto draw_segment_tree(Context& ctx, const Layout& layout, wire_id_t wire_id,
                       bool is_enabled, ElementDrawState state) -> void;

auto draw_segment_tree(Context& ctx, const Layout& layout, wire_id_t wire_id,
                       ElementDrawState state) -> void;

auto draw_wire(Context& ctx, const SpatialSimulation& spatial_simulation,
               wire_id_t wire_id) -> void;

auto draw_wires(Context& ctx, const Layout& layout, std::span<const wire_id_t> elements,
                ElementDrawState state) -> void;

auto draw_wires(Context& ctx, const SpatialSimulation& spatial_simulation,
                std::span<const wire_id_t> elements) -> void;

auto draw_wires(Context& ctx, std::span<const segment_info_t> segment_infos,
                ElementDrawState state) -> void;

}  // namespace logicsim

#endif
