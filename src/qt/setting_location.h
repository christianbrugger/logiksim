#ifndef LOGICSIM_QT_SETTING_LOCATION_H
#define LOGICSIM_QT_SETTING_LOCATION_H

#include <filesystem>

namespace logicsim {

enum class setting_t {
    gui_geometry,
    gui_state,

    logfile,
};

[[nodiscard]] auto get_writable_setting_path(setting_t settings) -> std::filesystem::path;

}  // namespace logicsim

#endif
