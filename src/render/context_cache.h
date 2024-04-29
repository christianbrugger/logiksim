#ifndef LOGICSIM_RENDER_CONTEXT_CACHE_H
#define LOGICSIM_RENDER_CONTEXT_CACHE_H

#include "render/svg_cache.h"
#include "render/text_cache.h"

namespace logicsim {

class TextCache;
class SVGCache;

namespace context_cache {
struct CacheData;
}

/**
 * @brief: Caches that are persistent across rendered frames.
 *
 * Note ContextCache is cheap to copy and can be passed by value.
 */
class ContextCache {
   public:
    [[nodiscard]] explicit ContextCache();
    [[nodiscard]] explicit ContextCache(FontFaces faces);

    [[nodiscard]] auto text_cache() const -> const TextCache&;
    [[nodiscard]] auto svg_cache() const -> const SVGCache&;

    /**
     * @brief: clear cached data
     */
    auto clear() -> void;
    /**
     * @brief: shrinks unused memory allocations of caches
     */
    auto shrink_to_fit() -> void;

   private:
    // const maintains whole-part relationship
    std::shared_ptr<const context_cache::CacheData> cache_;
};

}  // namespace logicsim

#endif
