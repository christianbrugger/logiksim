#ifndef LOGICSIM_SETTING_LOCATION_H
#define LOGICSIM_SETTING_LOCATION_H

#include <QString>

namespace logicsim {

enum class setting_t {
    gui_geometry,
    gui_state,

    logfile,
};

[[nodiscard]] auto get_writable_setting_path(setting_t settings) -> QString;

}  // namespace logicsim

#endif
