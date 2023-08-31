#ifndef LOGIKSIM_EDITABLE_CIRCUIT_CACHE_H
#define LOGIKSIM_EDITABLE_CIRCUIT_CACHE_H

#include "editable_circuit/cache/collision_cache.h"
#include "editable_circuit/cache/connection_cache.h"
#include "editable_circuit/cache/spatial_cache.h"

namespace logicsim {

class CacheProvider {
   public:
    CacheProvider() = default;
    explicit CacheProvider(const Layout& layout);

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto input_cache() const -> const ConnectionCache<true>&;
    [[nodiscard]] auto output_cache() const -> const ConnectionCache<false>&;
    [[nodiscard]] auto collision_cache() const -> const CollisionCache&;
    [[nodiscard]] auto spatial_cache() const -> const SpatialTree&;

    auto submit(editable_circuit::InfoMessage message) -> void;
    auto validate(const Layout& layout) -> void;

   public:
    // for rendering
    [[nodiscard]] auto input_positions() const {
        return input_connections_.positions();
    };

    [[nodiscard]] auto input_positions_and_orientations() const {
        return input_connections_.positions_and_orientations();
    };

    [[nodiscard]] auto output_positions() const {
        return output_connections_.positions();
    };

    [[nodiscard]] auto output_positions_and_orientations() const {
        return output_connections_.positions_and_orientations();
    };

    [[nodiscard]] auto collision_states() const {
        return collision_cache_.states();
    };

    [[nodiscard]] auto selection_rects() const {
        return spatial_cache_.rects();
    }

   private:
   private:
    ConnectionCache<true> input_connections_ {};
    ConnectionCache<false> output_connections_ {};
    CollisionCache collision_cache_ {};
    SpatialTree spatial_cache_ {};
};

static_assert(std::is_move_constructible_v<CollisionCache>);
static_assert(std::is_move_constructible_v<SpatialTree>);
static_assert(std::is_move_constructible_v<CacheProvider>);

}  // namespace logicsim

#endif