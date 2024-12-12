#include "core/render/context_cache.h"

#include "core/render/svg_cache.h"
#include "core/render/text_cache.h"
#include "core/vocabulary/allocation_info.h"

namespace logicsim {

namespace context_cache {

struct CacheData {
    TextCache text_cache {};
    SVGCache svg_cache {};
};

namespace {

auto create_cache_data(FontFaces faces) -> std::shared_ptr<const CacheData> {
    return std::make_shared<CacheData>(TextCache {std::move(faces)}, SVGCache {});
}

}  // namespace

}  // namespace context_cache

//
// Context Cache
//

ContextCache::ContextCache() : cache_ {std::make_shared<context_cache::CacheData>()} {}

ContextCache::ContextCache(FontFaces faces)
    : cache_ {context_cache::create_cache_data(std::move(faces))} {}

auto ContextCache::allocation_info() const -> ContextCacheAllocInfo {
    Expects(cache_ != nullptr);
    return ContextCacheAllocInfo {
        .text_cache = cache_->text_cache.allocation_info(), .svg_cache = {},  // ???
    };
}

auto ContextCache::text_cache() const -> const TextCache& {
    Expects(cache_ != nullptr);
    return cache_->text_cache;
}

auto ContextCache::svg_cache() const -> const SVGCache& {
    Expects(cache_ != nullptr);
    return cache_->svg_cache;
}

auto ContextCache::clear() -> void {
    Expects(cache_ != nullptr);

    cache_->text_cache.clear();
    cache_->svg_cache.clear();
}

//
// Free Functions
//

[[nodiscard]] auto cache_with_default_fonts() -> ContextCache {
    return ContextCache {FontFaces {get_default_font_locations()}};
}

}  // namespace logicsim
