#include "render/primitive/point.h"

#include "render/context.h"
#include "render/primitive/line.h"
#include "render/primitive/rect.h"
#include "vocabulary/color.h"
#include "vocabulary/grid_fine.h"
#include "vocabulary/point.h"
#include "vocabulary/rect_fine.h"

#include <gcem.hpp>

#include <exception>

namespace logicsim {

template <>
auto format(PointShape type) -> std::string {
    switch (type) {
        using enum PointShape;

        case circle:
            return "circle";
        case full_circle:
            return "full_circle";
        case cross:
            return "cross";
        case plus:
            return "plus";
        case square:
            return "square";
        case full_square:
            return "full_square";
        case diamond:
            return "diamond";
        case horizontal:
            return "horizontal";
        case vertical:
            return "vertical";
        case triangle_up:
            return "triangle_up";
    }
    std::terminate();
}

auto draw_point(Context& ctx, point_t point, PointShape shape, color_t color,
                grid_fine_t size) -> void {
    constexpr auto stroke_width = 1;

    switch (shape) {
        using enum PointShape;

        case circle: {
            const auto center = to_context(point, ctx);
            const auto r = to_context(size, ctx);

            ctx.bl_ctx.setStrokeWidth(stroke_width);
            ctx.bl_ctx.strokeCircle(BLCircle {center.x, center.y, r}, color);
            return;
        }
        case full_circle: {
            const auto center = to_context(point, ctx);
            const auto r = to_context(size, ctx);

            ctx.bl_ctx.fillCircle(BLCircle {center.x, center.y, r}, color);
            return;
        }
        case cross: {
            const auto [x, y] = to_context(point, ctx);
            const auto d = to_context(size, ctx);

            ctx.bl_ctx.setStrokeWidth(stroke_width);
            ctx.bl_ctx.strokeLine(BLLine {x - d, y - d, x + d, y + d}, color);
            ctx.bl_ctx.strokeLine(BLLine {x - d, y + d, x + d, y - d}, color);
            return;
        }
        case plus: {
            const auto [x, y] = to_context(point, ctx);
            const auto d = to_context(size, ctx);
            const auto attrs = LineAttributes {color, stroke_width};

            draw_orthogonal_line(ctx, BLLine {x, y + d, x, y - d}, attrs);
            draw_orthogonal_line(ctx, BLLine {x - d, y, x + d, y}, attrs);
            return;
        }
        case square: {
            draw_rect(ctx,
                      rect_fine_t {
                          point_fine_t {point.x - size, point.y - size},
                          point_fine_t {point.x + size, point.y + size},
                      },
                      RectAttributes {
                          .draw_type = ShapeDrawType::stroke,
                          .stroke_width = stroke_width,
                          .stroke_color = color,
                      });

            return;
        }
        case full_square: {
            draw_rect(ctx,
                      rect_fine_t {
                          point_fine_t {point.x - size, point.y - size},
                          point_fine_t {point.x + size, point.y + size},
                      },
                      RectAttributes {
                          .draw_type = ShapeDrawType::fill,
                          .stroke_width = stroke_width,
                          .fill_color = color,
                      });
            return;
        }
        case diamond: {
            const auto [x, y] = to_context(point, ctx);
            const auto d = to_context(size, ctx);

            const auto poly = std::array {BLPoint {x, y - d}, BLPoint {x + d, y},
                                          BLPoint {x, y + d}, BLPoint {x - d, y}};
            const auto view = BLArrayView<BLPoint> {poly.data(), poly.size()};

            ctx.bl_ctx.setStrokeWidth(stroke_width);
            ctx.bl_ctx.strokePolygon(BLArrayView<BLPoint>(view), color);
            return;
        }
        case horizontal: {
            const auto [x, y] = to_context(point, ctx);
            const auto d = to_context(size, ctx);
            const auto attrs = LineAttributes {color, stroke_width};

            draw_orthogonal_line(ctx, BLLine {x - d, y, x + d, y}, attrs);
            return;
        }
        case vertical: {
            const auto [x, y] = to_context(point, ctx);
            const auto d = to_context(size, ctx);
            const auto attrs = LineAttributes {color, stroke_width};

            draw_orthogonal_line(ctx, BLLine {x, y + d, x, y - d}, attrs);
            return;
        }
        case triangle_up: {
            const auto [x, y] = to_context(point, ctx);
            const auto d = to_context(size, ctx);
            // make all sides equal
            constexpr static auto h_factor = gcem::sqrt(3) - 1;
            const auto h = d * h_factor;

            const auto poly = std::array {
                BLPoint {x, y - d},
                BLPoint {x + d, y + h},
                BLPoint {x - d, y + h},
            };
            const auto view = BLArrayView<BLPoint> {poly.data(), poly.size()};

            ctx.bl_ctx.setStrokeWidth(stroke_width);
            ctx.bl_ctx.strokePolygon(BLArrayView<BLPoint>(view), color);
            return;
        }
    }
    std::terminate();
}

}  // namespace logicsim
