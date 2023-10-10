#ifndef LOGIKSIM_SVG_CACHE_H
#define LOGIKSIM_SVG_CACHE_H

#include "resource.h"
#include "vocabulary/alignment.h"
#include "vocabulary/color.h"

#include <ankerl/unordered_dense.h>
#include <blend2d.h>
#include <gsl/gsl>

#include <memory>

namespace logicsim {

namespace svg_cache {

struct svg_data_t;

struct svg_entry_t {
    std::unique_ptr<const svg_data_t> data {};  // Pimpl

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
        double height {100};  // pixel

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
