#ifndef LOGICSIM_VOCABULARY_WIDGET_RENDER_CONFIG_H
#define LOGICSIM_VOCABULARY_WIDGET_RENDER_CONFIG_H

#include "format/struct.h"
#include "vocabulary/thread_count.h"
#include "vocabulary/wire_render_style.h"

#include <type_traits>

namespace logicsim {

struct WidgetRenderConfig {
    ThreadCount thread_count {ThreadCount::four};
    WireRenderStyle wire_render_style {WireRenderStyle::bold_red};

    bool do_benchmark {false};
    bool show_circuit {true};
    bool show_collision_cache {false};
    bool show_connection_cache {false};
    bool show_selection_cache {false};

    bool show_render_borders {false};
    bool show_mouse_position {false};
    bool direct_rendering {true};
    bool jit_rendering {true};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const WidgetRenderConfig &) const -> bool = default;
};

static_assert(std::is_aggregate_v<WidgetRenderConfig>);
static_assert(std::regular<WidgetRenderConfig>);

}  // namespace logicsim

#endif
