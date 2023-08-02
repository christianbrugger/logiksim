
#include "editable_circuit/caches.h"

namespace logicsim {

//
// CacheProvider
//

CacheProvider::CacheProvider(const Layout& layout) {
    // TODO consider bulk insertion, especially for spatial_cache_
    add_layout_to_cache(input_connections_, layout);
    add_layout_to_cache(output_connections_, layout);
    add_layout_to_cache(collision_cache_, layout);
    add_layout_to_cache(spatial_cache_, layout);
}

auto CacheProvider::format() const -> std::string {
    return fmt::format(
        "EditableCircuit::CacheProvider{{\n"
        "{}\n{}\n{}\n{}\n"
        "}}\n",
        input_connections_, output_connections_, collision_cache_, spatial_cache_);
}

auto CacheProvider::validate(const Layout& layout) -> void {
    spatial_cache_.validate(layout);
    collision_cache_.validate(layout);
    input_connections_.validate(layout);
    output_connections_.validate(layout);
}

auto CacheProvider::submit(editable_circuit::InfoMessage message) -> void {
    input_connections_.submit(message);
    output_connections_.submit(message);
    collision_cache_.submit(message);
    spatial_cache_.submit(message);
}

auto CacheProvider::input_cache() const -> const ConnectionCache<true>& {
    return input_connections_;
}

auto CacheProvider::output_cache() const -> const ConnectionCache<false>& {
    return output_connections_;
}

auto CacheProvider::collision_cache() const -> const CollisionCache& {
    return collision_cache_;
}

auto CacheProvider::spatial_cache() const -> const SpatialTree& {
    return spatial_cache_;
}

}  // namespace logicsim
