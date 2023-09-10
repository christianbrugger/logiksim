#ifndef LOGIKSIM_SVG_CACHE_H
#define LOGIKSIM_SVG_CACHE_H

#include "glyph_cache_type.h"  // TODO move HorizontalAlignment to different header?
#include "resource.h"

#include <ankerl/unordered_dense.h>
#include <gsl/gsl>

#include <memory>

class BLContext;

namespace logicsim {

namespace svg_cache {

struct svg_data_t;

struct svg_entry_t {
    std::unique_ptr<const svg_data_t> data {};

    [[nodiscard]] explicit svg_entry_t();
    [[nodiscard]] explicit svg_entry_t(svg_data_t &&data);

    [[nodiscard]] explicit svg_entry_t(svg_entry_t &&);
    [[nodiscard]] explicit svg_entry_t(const svg_entry_t &) = delete;
    [[nodiscard]] auto operator=(svg_entry_t &&) -> svg_entry_t &;
    [[nodiscard]] auto operator=(const svg_entry_t &) -> svg_entry_t & = delete;
    ~svg_entry_t();
};

}  // namespace svg_cache

class SVGCache {
   private:
    using svg_data_t = svg_cache::svg_data_t;
    using svg_entry_t = svg_cache::svg_entry_t;

   public:
    [[nodiscard]] explicit SVGCache() = default;

    auto clear() -> void;
    auto shrink_to_fit() -> void;

    struct IconAttributes {
        icon_t icon;
        BLPoint position {};
        double height {24};

        color_t color {defaults::color_black};
        HorizontalAlignment horizontal_alignment {HorizontalAlignment::left};
        VerticalAlignment vertical_alignment {VerticalAlignment::top};
    };

    auto draw_icon(BLContext &bl_ctx, IconAttributes attributes) const -> void;

   private:
    auto get_entry(icon_t icon) const -> const svg_entry_t &;

   private:
    using svg_map_t = ankerl::unordered_dense::map<icon_t, svg_entry_t>;

    mutable svg_map_t svg_map_ {};
};

}  // namespace logicsim

#endif
