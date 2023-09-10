#include "svg_cache.h"

#include "blend2d.h"
#include "file.h"
#include "format.h"
#include "render_helper.h"
#include "resource.h"
#include "timer.h"
#include "vocabulary.h"

#include <svgshapes.h>

namespace logicsim {

namespace svg_cache {

struct svg_data_t {
    svg2b2d::SVGDocument document {};
};

svg_entry_t::svg_entry_t() = default;

svg_entry_t::svg_entry_t(svg_data_t &&data)
    : data {std::make_unique<svg_data_t>(std::move(data))} {}

svg_entry_t::svg_entry_t(svg_entry_t &&) = default;

auto svg_entry_t::operator=(svg_entry_t &&) -> svg_entry_t & = default;

svg_entry_t::~svg_entry_t() = default;

}  // namespace svg_cache

namespace {

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

auto SVGCache::draw_icon(BLContext &bl_ctx, IconAttributes attributes) const -> void {
    const auto &entry = get_entry(attributes.icon);

    if (!entry.data) {
        return;
    }
    const auto &document = entry.data->document;

    if (document.height() <= 0) {
        return;
    }
    const auto scale = attributes.height / document.height();

    render_svg_icon_impl(bl_ctx, document, attributes.position, attributes.color, scale);
}

namespace {

auto load_svg_icon(icon_t icon) -> svg2b2d::SVGDocument {
    const auto binary = load_file(get_icon_path(icon));

    auto byte_span = svg2b2d::ByteSpan {binary.data(), binary.size()};
    auto document = svg2b2d::SVGDocument {};
    document.readFromData(byte_span);

    return document;
}

}  // namespace

auto SVGCache::get_entry(icon_t icon) const -> const svg_entry_t & {
    if (const auto it = svg_map_.find(icon); it != svg_map_.end()) {
        return it->second;
    }

    // insert
    const auto [it, _] =
        svg_map_.emplace(icon, svg_entry_t {svg_data_t {load_svg_icon(icon)}});

    return it->second;
}

}  // namespace logicsim