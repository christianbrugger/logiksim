#include "setting_location.h"

#include "resource.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace logicsim {

namespace {

[[nodiscard]] auto writable_standard_path(QStandardPaths::StandardLocation location,
                                          QString relative) -> QString {
    const auto folder = QStandardPaths::writableLocation(location);
    const auto file = QFileInfo(folder + '/' + LS_APP_VERSION_STR + '/' + relative);
    file.absoluteDir().mkpath(".");
    return file.absoluteFilePath();
}

}  // namespace

auto get_writable_setting_path(setting_t settings) -> QString {
    switch (settings) {
        using enum setting_t;

        case gui_geometry: {
            return writable_standard_path(QStandardPaths::AppConfigLocation,
                                          "gui_geometry.bin");
        }
        case gui_state: {
            return writable_standard_path(QStandardPaths::AppConfigLocation,
                                          "gui_state.bin");
        }
        case logfile: {
            return writable_standard_path(QStandardPaths::AppConfigLocation,
                                          "logging.txt");
        }
    };
    std::terminate();
}

}  // namespace logicsim
