#ifndef LOGICSIM_COMPONENT_INTERACTIVE_SIMULATION_INTERACTION_CACHE_H
#define LOGICSIM_COMPONENT_INTERACTIVE_SIMULATION_INTERACTION_CACHE_H

#include "format/struct.h"
#include "vocabulary/element_id.h"
#include "vocabulary/point.h"

#include <ankerl/unordered_dense.h>

#include <optional>

namespace logicsim {

class Layout;

namespace interactive_simulation {

struct interaction_data_t {
    element_id_t element_id {null_element};

    auto operator==(const interaction_data_t& other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

class InteractionCache {
   public:
    using map_type = ankerl::unordered_dense::map<point_t, interaction_data_t>;

   public:
    explicit InteractionCache(const Layout& layout);

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto find(point_t position) const -> std::optional<element_id_t>;

   private:
    map_type map_ {};
};

}  // namespace interactive_simulation

}  // namespace logicsim

#endif
