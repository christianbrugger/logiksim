#include "render/svg_cache.h"

#include "file.h"
#include "logging.h"
#include "render/context_guard.h"
#include "resource.h"

#include <exception>
#include <stdexcept>
#include <svgshapes.h>

namespace logicsim {

namespace svg_cache {

struct svg_data_t {
    svg2b2d::SVGDocument document {};
};

}  // namespace svg_cache

template class value_pointer<const svg_cache::svg_data_t>;

namespace {

auto load_svg_icon(icon_t icon) -> svg_cache::svg_data_t {
    const auto filename = get_icon_path(icon);
    const auto binary = load_file(filename);

    if (binary.empty()) {
        print("WARNING: unable to load svg icon", filename);
    }

    auto byte_span = svg2b2d::ByteSpan {binary.data(), binary.size()};
    auto result = svg_cache::svg_data_t {};
    result.document.readFromData(byte_span);

    return result;
}

auto render_svg_icon_impl(BLContext &bl_ctx, const svg2b2d::SVGDocument &document,
                          BLPoint position, color_t color, double scale) -> void {
    auto _ [[maybe_unused]] = make_context_guard(bl_ctx);

    bl_ctx.translate(position);
    bl_ctx.scale(scale);
    bl_ctx.setStrokeStyle(color);

    bl_ctx.setFillStyle(color);
    bl_ctx.setStrokeStyle(color);

    document.draw(bl_ctx);
}

}  // namespace

auto SVGCache::clear() -> void {
    svg_map_.clear();
}

auto SVGCache::shrink_to_fit() -> void {
    svg_map_.rehash(svg_map_.size());
}

namespace {

auto calculate_offset_x(double width, HorizontalAlignment horizontal_alignment)
    -> double {
    switch (horizontal_alignment) {
        using enum HorizontalAlignment;

        case left:
            return 0;
        case right:
            return -width;
        case center:
            return -width / 2.0;
    }
    std::terminate();
}

auto calculate_offset_y(double height, VerticalAlignment vertical_alignment) -> double {
    switch (vertical_alignment) {
        using enum VerticalAlignment;

        case top:
            return 0;
        case bottom:
            return -height;
        case center:
            return -height / 2.0;
    }
    std::terminate();
}

auto calculate_offset(const svg2b2d::SVGDocument &document, double scale,
                      HorizontalAlignment horizontal_alignment,
                      VerticalAlignment vertical_alignment) -> BLPoint {
    return BLPoint {
        calculate_offset_x(document.width(), horizontal_alignment) * scale,
        calculate_offset_y(document.height(), vertical_alignment) * scale,
    };
}

}  // namespace

auto SVGCache::draw_icon(BLContext &bl_ctx, IconAttributes attributes) const -> void {
    const auto &entry = get_svg_data(attributes.icon);
    const auto &document = entry.document;

    if (document.height() <= 0 || document.width() <= 0) {
        return;
    }

    const auto scale = attributes.height / document.height();
    const auto offset = calculate_offset(document, scale, attributes.horizontal_alignment,
                                         attributes.vertical_alignment);
    const auto position = attributes.position + offset;

    render_svg_icon_impl(bl_ctx, document, position, attributes.color, scale);
}

namespace {}  // namespace

auto SVGCache::get_svg_data(icon_t icon) const -> const svg_data_t & {
    if (const auto it = svg_map_.find(icon); it != svg_map_.end()) {
        return *it->second;
    }

    // insert
    const auto [it, _] = svg_map_.emplace(icon, svg_entry_t {load_svg_icon(icon)});

    return *it->second;
}

}  // namespace logicsim