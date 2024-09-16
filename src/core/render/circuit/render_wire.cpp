#include "render/circuit/render_wire.h"

#include "component/simulation/history_view.h"
#include "geometry/interpolation.h"
#include "layout.h"
#include "render/circuit/alpha_values.h"
#include "render/context.h"
#include "render/primitive/line.h"
#include "spatial_simulation.h"
#include "vocabulary/color.h"
#include "vocabulary/line_fine.h"

#include <fmt/core.h>

namespace logicsim {

namespace defaults {
constexpr static inline auto wire_color_disabled = defaults::color_black;
constexpr static inline auto wire_color_enabled = defaults::color_red;
}  // namespace defaults

auto wire_color(bool is_enabled) -> color_t {
    if (is_enabled) {
        return defaults::wire_color_enabled;
    }
    return defaults::wire_color_disabled;
}

auto wire_color(bool is_enabled, ElementDrawState state) -> color_t {
    return with_alpha_runtime(wire_color(is_enabled), state);
}

auto draw_line_cross_point(Context& ctx, point_t point, bool is_enabled,
                           ElementDrawState state) -> void {
    int lc_width = ctx.view_config().line_cross_width();
    if (lc_width <= 0) {
        return;
    }

    const int wire_width = ctx.view_config().stroke_width();
    const int wire_offset = (wire_width - 1) / 2;

    const int size = 2 * lc_width + wire_width;
    const int offset = wire_offset + lc_width;

    const auto [x, y] = to_context(point, ctx);
    const auto color = wire_color(is_enabled, state);

    ctx.bl_ctx.fillRect(BLRect {x - offset, y - offset, 1. * size, 1. * size}, color);
}

auto SegmentAttributes::format() const -> std::string {
    return fmt::format(
        "SegmentAttributes(is_enabled = {}, p0_endcap = {}, p1_endcap = {})", is_enabled,
        p0_endcap, p1_endcap);
}

auto draw_line_segment(Context& ctx, line_fine_t line, SegmentAttributes attributes,
                       ElementDrawState state) -> void {
    const auto color = wire_color(attributes.is_enabled, state);
    draw_line(ctx, line,
              LineAttributes {
                  .color = color,
                  .p0_endcap = attributes.p0_endcap,
                  .p1_endcap = attributes.p1_endcap,
              });
}

auto draw_line_segment(Context& ctx, ordered_line_t line, SegmentAttributes attributes,
                       ElementDrawState state) -> void {
    draw_line_segment(ctx, line_fine_t {line}, attributes, state);
}

auto draw_line_segment(Context& ctx, segment_info_t info, bool is_enabled,
                       ElementDrawState state) -> void {
    draw_line_segment(ctx, info.line,
                      SegmentAttributes {
                          .is_enabled = is_enabled,
                          .p0_endcap = info.p0_type == SegmentPointType::corner_point,
                          .p1_endcap = info.p1_type == SegmentPointType::corner_point,
                      },
                      state);

    if (is_cross_point(info.p0_type)) {
        draw_line_cross_point(ctx, info.line.p0, is_enabled, state);
    }
    if (is_cross_point(info.p1_type)) {
        draw_line_cross_point(ctx, info.line.p1, is_enabled, state);
    }
}

auto draw_segment_tree(Context& ctx, const Layout& layout, wire_id_t wire_id,
                       bool is_enabled, ElementDrawState state) -> void {
    for (const segment_info_t& info : layout.wires().segment_tree(wire_id)) {
        draw_line_segment(ctx, info, is_enabled, state);
    }
}

auto draw_segment_tree(Context& ctx, const Layout& layout, wire_id_t wire_id,
                       ElementDrawState state) -> void {
    bool is_enabled = false;
    draw_segment_tree(ctx, layout, wire_id, is_enabled, state);
}

auto _draw_line_segment_with_history(Context& ctx, point_t p_from, point_t p_until,
                                     time_t time_from, time_t time_until,
                                     const simulation::HistoryView& history,
                                     bool p0_is_corner, bool p1_is_corner) -> void {
    assert(time_from < time_until);

    const auto it_from = history.from(time_from);
    const auto it_until = history.until(time_until);

    for (const auto& entry : std::ranges::subrange(it_from, it_until)) {
        const auto p_start =
            interpolate_line_1d(p_from, p_until, time_from, time_until, entry.first_time);
        const auto p_end =
            interpolate_line_1d(p_from, p_until, time_from, time_until, entry.last_time);

        if (p_start != p_end) [[likely]] {
            draw_line_segment(
                ctx, line_fine_t {p_start, p_end},
                {
                    .is_enabled = entry.value,
                    .p0_endcap = p0_is_corner && (p_start == point_fine_t {p_from}),
                    .p1_endcap = p1_is_corner && (p_end == point_fine_t {p_until}),
                },
                ElementDrawState::normal);
        }
    }
}

auto _draw_wire_with_history(Context& ctx, const LineTree& line_tree,
                             simulation::HistoryView history,
                             delay_t wire_delay_per_distance) -> void {
    if (history.size() < 2) [[unlikely]] {
        throw std::runtime_error("requires history view with at least 2 entries");
    }

    const auto to_time = [time = history.simulation_time(),
                          delay = wire_delay_per_distance](length_t length_) {
        // TODO add * operator to delay_t ?
        return time - length_.value * delay;
    };

    for (auto&& index : indices(line_tree)) {
        const auto line = line_tree.line(index);
        _draw_line_segment_with_history(ctx,                                  //
                                        line.p1,                              //
                                        line.p0,                              //
                                        to_time(line_tree.length_p1(index)),  //
                                        to_time(line_tree.length_p0(index)),  //
                                        history,                              //
                                        line_tree.is_corner_p1(index),        //
                                        line_tree.is_corner_p0(index)         //
        );

        if (line_tree.has_cross_point_p0(index)) {
            bool wire_enabled = history.value(to_time(line_tree.length_p0(index)));
            draw_line_cross_point(ctx, line.p0, wire_enabled, ElementDrawState::normal);
        }
    }
}

auto draw_wire(Context& ctx, const SpatialSimulation& spatial_simulation,
               wire_id_t wire_id) -> void {
    const auto element_id = to_element_id(spatial_simulation, wire_id);
    const auto history = spatial_simulation.simulation().input_history(element_id);

    if (history.size() <= 1) {
        draw_segment_tree(ctx, spatial_simulation.layout(), wire_id, history.last_value(),
                          ElementDrawState::normal);
        return;
    }

    _draw_wire_with_history(ctx, spatial_simulation.line_tree(wire_id), history,
                            spatial_simulation.wire_delay_per_distance());
}

auto draw_wires(Context& ctx, const Layout& layout, std::span<const wire_id_t> elements,
                ElementDrawState state) -> void {
    for (const auto& wire_id : elements) {
        draw_segment_tree(ctx, layout, wire_id, state);
    }
}

auto draw_wires(Context& ctx, const SpatialSimulation& spatial_simulation,
                std::span<const wire_id_t> elements) -> void {
    for (const auto& wire_id : elements) {
        draw_wire(ctx, spatial_simulation, wire_id);
    }
}

auto draw_wires(Context& ctx, std::span<const segment_info_t> segment_infos,
                ElementDrawState state) -> void {
    for (const auto& info : segment_infos) {
        const auto is_enabled = false;
        draw_line_segment(ctx, info, is_enabled, state);
    }
}

}  // namespace logicsim
