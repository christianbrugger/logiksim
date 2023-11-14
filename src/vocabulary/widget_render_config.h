#ifndef LOGICSIM_VOCABULARY_WIDGET_RENDER_CONFIG_H
#define LOGICSIM_VOCABULARY_WIDGET_RENDER_CONFIG_H

#include "format/struct.h"
#include <type_traits>

namespace logicsim {

struct WidgetRenderConfig {
    bool do_benchmark {false};
    bool show_circuit {true};
    bool show_collision_cache {false};
    bool show_connection_cache {false};
    bool show_selection_cache {false};

    double zoom_level {1};

    int thread_count {4};
    bool direct_rendering {true};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const WidgetRenderConfig &) const -> bool = default;
};

static_assert(std::regular<WidgetRenderConfig>);

}  // namespace logicsim

#endif