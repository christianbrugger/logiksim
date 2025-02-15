#include "core/render/circuit/render_layout_index.h"

#include "core/editable_circuit.h"
#include "core/geometry/orientation.h"
#include "core/geometry/rect.h"
#include "core/geometry/scene.h"
#include "core/render/context.h"
#include "core/render/context_guard.h"
#include "core/render/primitive/arrow.h"
#include "core/render/primitive/point.h"
#include "core/render/primitive/rect.h"
#include "core/vocabulary/color.h"

#include <numbers>
#include <stdexcept>

namespace logicsim {

namespace {

auto _directed_input_marker(Context& ctx, point_t point, color_t color,
                            orientation_t orientation, grid_fine_t size) -> void {
    auto _ [[maybe_unused]] = make_context_guard(ctx);

    const auto [x, y] = to_context(point, ctx);
    const auto d = to_context(size, ctx);
    const auto angle = to_angle(orientation);

    ctx.bl_ctx.translate(BLPoint {x, y});
    ctx.bl_ctx.rotate(angle);

    const auto pi = std::numbers::pi;

    ctx.bl_ctx.setStrokeWidth(1);
    ctx.bl_ctx.strokeArc(BLArc {0, 0, d, d, -pi / 2, pi}, color);
    ctx.bl_ctx.strokeLine(BLLine {-d, -d, 0, -d}, color);
    ctx.bl_ctx.strokeLine(BLLine {-d, +d, 0, +d}, color);
}

auto _undirected_input_marker(Context& ctx, point_t point, color_t color,
                              grid_fine_t size) -> void {
    auto _ [[maybe_unused]] = make_context_guard(ctx);

    ctx.bl_ctx.setStrokeWidth(1);
    ctx.bl_ctx.setStrokeStyle(color);

    const auto [x, y] = to_context(point, ctx);
    const auto d = to_context(size, ctx);
    const auto h = d / 2;

    ctx.bl_ctx.translate(BLPoint {x + 0.5, y + 0.5});

    ctx.bl_ctx.strokeLine(BLLine {-d, -d, -h, -d});
    ctx.bl_ctx.strokeLine(BLLine {+h, -d, +d, -d});

    ctx.bl_ctx.strokeLine(BLLine {-d, -d, -d, -h});
    ctx.bl_ctx.strokeLine(BLLine {-d, +h, -d, +d});

    ctx.bl_ctx.strokeLine(BLLine {+d, -d, +d, -h});
    ctx.bl_ctx.strokeLine(BLLine {+d, +h, +d, +d});

    ctx.bl_ctx.strokeLine(BLLine {-d, +d, -h, +d});
    ctx.bl_ctx.strokeLine(BLLine {+h, +d, +d, +d});
}

auto render_input_marker(Context& ctx, point_t point, color_t color,
                         orientation_t orientation, grid_fine_t size) -> void {
    if (orientation == orientation_t::undirected) {
        _undirected_input_marker(ctx, point, color, size);
    } else {
        _directed_input_marker(ctx, point, color, orientation, size);
    }
}

auto render_undirected_output(Context& ctx, point_t position, grid_fine_t size,
                              color_t color) {
    draw_point(ctx, position, PointShape::cross, color, size / 4);
    draw_point(ctx, position, PointShape::plus, color, size / 3);
}

auto render_output_marker(Context& ctx, point_t position, color_t color,
                          orientation_t orientation, grid_fine_t size) -> void {
    if (orientation == orientation_t::undirected) {
        render_undirected_output(ctx, position, size, color);
    } else {
        draw_arrow(ctx, position, color, orientation, size);
    }
}

}  // namespace

auto render_layout_connection_index(Context& ctx, const LayoutIndex& index) -> void {
    const auto scene_rect = get_scene_rect(ctx.settings.view_config);

    const auto logicitem_color = defaults::color_dark_blue;
    const auto wire_color = defaults::color_green;

    const auto input_size = grid_fine_t {1.0 / 3.0};
    const auto output_size = grid_fine_t {0.8};

    // input
    for (const auto& [position, orientation] :
         index.logicitem_input_index().positions_and_orientations()) {
        if (is_colliding(position, scene_rect)) {
            render_input_marker(ctx, position, logicitem_color, orientation, input_size);
        }
    }
    for (const auto& [position, orientation] :
         index.wire_input_index().positions_and_orientations()) {
        if (is_colliding(position, scene_rect)) {
            render_input_marker(ctx, position, wire_color, orientation, input_size);
        }
    }

    // output
    for (const auto& [position, orientation] :
         index.logicitem_output_index().positions_and_orientations()) {
        if (is_colliding(position, scene_rect)) {
            render_output_marker(ctx, position, logicitem_color, orientation,
                                 output_size);
        }
    }
    for (const auto& [position, orientation] :
         index.wire_output_index().positions_and_orientations()) {
        if (is_colliding(position, scene_rect)) {
            render_output_marker(ctx, position, wire_color, orientation, output_size);
        }
    }
}

auto render_layout_collision_index(Context& ctx,
                                   const CollisionIndex& collision_index) -> void {
    constexpr static auto color = defaults::color_orange;
    constexpr static auto size = grid_fine_t {0.25};

    const auto scene_rect = get_scene_rect(ctx.settings.view_config);

    for (const auto& [point, state] : collision_index.states()) {
        if (!is_colliding(point, scene_rect)) {
            continue;
        }

        switch (state) {
            using enum collision_index::IndexState;

            case logicitem_body: {
                draw_point(ctx, point, PointShape::square, color, size);
                break;
            }
            case logicitem_connection: {
                draw_point(ctx, point, PointShape::circle, color, size);
                break;
            }
            case decoration: {
                draw_point(ctx, point, PointShape::triangle_up, color, size);
                break;
            }
            case wire_connection: {
                draw_point(ctx, point, PointShape::full_square, color, size * (2. / 3));
                break;
            }
            case wire_horizontal: {
                draw_point(ctx, point, PointShape::horizontal, color, size);
                break;
            }
            case wire_vertical: {
                draw_point(ctx, point, PointShape::vertical, color, size);
                break;
            }
            case wire_corner_point: {
                draw_point(ctx, point, PointShape::diamond, color, size);
                break;
            }
            case wire_cross_point: {
                draw_point(ctx, point, PointShape::cross, color, size);
                break;
            }
            case wire_crossing: {
                draw_point(ctx, point, PointShape::plus, color, size);
                break;
            }
            case logicitem_wire_connection: {
                draw_point(ctx, point, PointShape::full_circle, color, size);
                break;
            }
        }
    }
}

auto render_layout_selection_index(Context& ctx,
                                   const SpatialIndex& selection_index) -> void {
    const auto scene_rect = get_scene_rect_fine(ctx.settings.view_config);

    for (const rect_fine_t& rect : selection_index.rects()) {
        if (!is_colliding(rect, scene_rect)) {
            continue;
        }

        draw_rect(ctx, rect,
                  RectAttributes {
                      .draw_type = ShapeDrawType::stroke,
                      .stroke_width = 1,
                      .stroke_color = defaults::color_lime,
                  });
    }
}

auto render_layout_connection_index(Context& ctx,
                                    const EditableCircuit& editable_circuit) -> void {
    render_layout_connection_index(ctx, editable_circuit.modifier().circuit_data().index);
}

auto render_layout_collision_index(Context& ctx,
                                   const EditableCircuit& editable_circuit) -> void {
    render_layout_collision_index(
        ctx, editable_circuit.modifier().circuit_data().index.collision_index());
}

auto render_layout_selection_index(Context& ctx,
                                   const EditableCircuit& editable_circuit) -> void {
    render_layout_selection_index(
        ctx, editable_circuit.modifier().circuit_data().index.selection_index());
}

}  // namespace logicsim
