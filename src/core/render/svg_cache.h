#ifndef LOGIKSIM_RENDER_SVG_CACHE_H
#define LOGIKSIM_RENDER_SVG_CACHE_H

#include "container/value_pointer.h"
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
using svg_entry_t = value_pointer<const svg_data_t>;
using svg_map_t = ankerl::unordered_dense::map<icon_t, svg_entry_t>;

}  // namespace svg_cache

extern template class value_pointer<const svg_cache::svg_data_t>;

class SVGCache {
   private:
    using svg_data_t = svg_cache::svg_data_t;
    using svg_entry_t = svg_cache::svg_entry_t;
    using svg_map_t = svg_cache::svg_map_t;

   public:
    [[nodiscard]] explicit SVGCache() = default;

    [[nodiscard]] auto operator==(const SVGCache &) const noexcept -> bool;

    auto clear() const -> void;

    struct IconAttributes {
        icon_t icon {icon_t::app_icon};
        BLPoint position {};
        double height {100};  // pixel

        color_t color {defaults::color_black};
        HorizontalAlignment horizontal_alignment {HorizontalAlignment::left};
        VerticalAlignment vertical_alignment {VerticalAlignment::top};
    };

    auto draw_icon(BLContext &bl_ctx, IconAttributes attributes) const -> void;

   private:
    mutable svg_map_t svg_map_ {};
};

static_assert(std::regular<SVGCache>);

}  // namespace logicsim

#endif
