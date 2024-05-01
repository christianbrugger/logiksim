#ifndef LOGICSIM_RENDER_CONTEXT_CACHE_H
#define LOGICSIM_RENDER_CONTEXT_CACHE_H

#include <memory>
#include <concepts>

namespace logicsim {

class TextCache;
class SVGCache;
struct FontFaces;

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
    [[nodiscard]] ContextCache();
    [[nodiscard]] explicit ContextCache(FontFaces faces);

    [[nodiscard]] auto text_cache() const -> const TextCache&;
    [[nodiscard]] auto svg_cache() const -> const SVGCache&;

    /**
     * @brief: clear cached data and de-allocates storages
     */
    auto clear() -> void;

   private:
    // const maintains whole-part relationship
    std::shared_ptr<const context_cache::CacheData> cache_;
};

static_assert(std::semiregular<ContextCache>);

[[nodiscard]] auto cache_with_default_fonts() -> ContextCache;

}  // namespace logicsim

#endif
