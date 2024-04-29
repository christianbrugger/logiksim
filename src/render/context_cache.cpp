#include "context_cache.h"

#include "render/context_cache.h"
#include "render/svg_cache.h"
#include "render/text_cache.h"

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

auto ContextCache::shrink_to_fit() -> void {
    Expects(cache_ != nullptr);

    cache_->text_cache.shrink_to_fit();
    cache_->svg_cache.shrink_to_fit();
}

}  // namespace logicsim
