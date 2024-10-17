#ifndef LOGICSIM_CORE_SERIALIZE_GUI_SETTING_H
#define LOGICSIM_CORE_SERIALIZE_GUI_SETTING_H

#include "core/format/struct.h"
#include "core/vocabulary/load_error.h"
#include "core/vocabulary/thread_count.h"
#include "core/vocabulary/wire_render_style.h"

#include <tl/expected.hpp>

#include <optional>
#include <string>
#include <type_traits>

namespace logicsim {

/**
 * @brief: GUI settings file version, always increasing.
 *
 * 200: LogikSim 2.2.0
 */
constexpr static inline auto CURRENT_GUI_SETTING_VERSION = 200;

/*
 * @brief: Presistent GUI settings that are stored on disk.
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
    -> tl::expected<GuiSettings, LoadError>;

}  // namespace logicsim

#endif
