#ifndef LOGICSIM_CORE_SERIALIZE_GUI_SETTING_H
#define LOGICSIM_CORE_SERIALIZE_GUI_SETTING_H

#include "format/struct.h"
#include "vocabulary/thread_count.h"
#include "vocabulary/wire_render_style.h"

#include <optional>
#include <string>
#include <type_traits>

namespace logicsim {

constexpr static inline auto CURRENT_GUI_SETTING_VERSION = 200;

/*
 * @brief: Presistent GUI settings that are stored as settings.
 */
struct GuiSettings {
    int version {CURRENT_GUI_SETTING_VERSION};

    ThreadCount thread_count;
    WireRenderStyle wire_render_style;
    bool direct_rendering;
    bool jit_rendering;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const GuiSettings&) const -> bool = default;
};

static_assert(std::is_aggregate_v<GuiSettings>);
static_assert(std::regular<GuiSettings>);

/**
 * @brief: Serialize the Gui settings as json.
 */
[[nodiscard]] auto serialize_gui_settings(const GuiSettings& settings) -> std::string;

/**
 * @brief: Load Gui settings from serialized json.
 */
[[nodiscard]] auto load_gui_settings(const std::string& text)
    -> std::optional<GuiSettings>;

}  // namespace logicsim

#endif
