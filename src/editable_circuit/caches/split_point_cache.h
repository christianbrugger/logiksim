#ifndef LOGIKSIM_EDITABLE_CIRCUIT_CACHES_SPLIT_POINT_CACHE_H
#define LOGIKSIM_EDITABLE_CIRCUIT_CACHES_SPLIT_POINT_CACHE_H

#include "vocabulary.h"

namespace logicsim {

namespace detail::split_point_cache {
struct tree_container;
}

class SplitPointCache {
   public:
    explicit SplitPointCache();
    explicit SplitPointCache(std::span<const point_t> points);
    ~SplitPointCache();
    SplitPointCache(SplitPointCache &&);
    auto operator=(SplitPointCache &&) -> SplitPointCache &;

    auto format() const -> std::string;

    auto add_split_point(point_t point) -> void;

    auto query_is_inside(ordered_line_t line, std::vector<point_t> &result) const -> void;
    auto query_intersects(ordered_line_t line, std::vector<point_t> &result) const
        -> void;

   private:
    std::unique_ptr<detail::split_point_cache::tree_container> tree_;
};

}  // namespace logicsim

#endif
