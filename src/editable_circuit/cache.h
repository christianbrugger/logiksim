#ifndef LOGIKSIM_EDITABLE_CIRCUIT_CACHE_H
#define LOGIKSIM_EDITABLE_CIRCUIT_CACHE_H

#include "editable_circuit/cache/collision_cache.h"
#include "editable_circuit/cache/connection_cache.h"
#include "editable_circuit/cache/spatial_cache.h"
#include "format/struct.h"

namespace logicsim {

class CacheProvider {
   public:
    CacheProvider() = default;
    explicit CacheProvider(const Layout& layout);

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    [[nodiscard]] auto logicitem_input_cache() const -> const LogicItemInputCache&;
    [[nodiscard]] auto logicitem_output_cache() const -> const LogicItemOutputCache&;
    [[nodiscard]] auto wire_input_cache() const -> const WireInputCache&;
    [[nodiscard]] auto wire_output_cache() const -> const WireOutputCache&;

    [[nodiscard]] auto collision_cache() const -> const CollisionCache&;
    [[nodiscard]] auto spatial_cache() const -> const SpatialTree&;

    auto submit(const editable_circuit::InfoMessage& message) -> void;
    auto validate(const Layout& layout) -> void;

   public:
    // for rendering
    [[nodiscard]] auto logicitem_input_positions_and_orientations() const {
        return logicitems_inputs_.positions_and_orientations();
    };

    [[nodiscard]] auto logicitem_output_positions_and_orientations() const {
        return logicitems_outputs_.positions_and_orientations();
    };

    [[nodiscard]] auto wire_input_positions_and_orientations() const {
        return wire_inputs_.positions_and_orientations();
    };

    [[nodiscard]] auto wire_output_positions_and_orientations() const {
        return wire_outputs_.positions_and_orientations();
    };

    [[nodiscard]] auto collision_states() const {
        return collision_cache_.states();
    };

    [[nodiscard]] auto selection_rects() const {
        return spatial_cache_.rects();
    }

   private:
    LogicItemInputCache logicitems_inputs_ {};
    LogicItemOutputCache logicitems_outputs_ {};
    WireInputCache wire_inputs_ {};
    WireOutputCache wire_outputs_ {};

    CollisionCache collision_cache_ {};
    SpatialTree spatial_cache_ {};
};

static_assert(std::is_move_constructible_v<CollisionCache>);
static_assert(std::is_move_constructible_v<SpatialTree>);
static_assert(std::is_move_constructible_v<CacheProvider>);

}  // namespace logicsim

#endif
