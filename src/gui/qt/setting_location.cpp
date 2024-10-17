#include "gui/qt/setting_location.h"

#include "gui/qt/path_conversion.h"

#include "core/resource.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace logicsim {

namespace {

[[nodiscard]] auto writable_standard_path(QStandardPaths::StandardLocation location,
                                          const std::filesystem::path &relative)
    -> std::filesystem::path {
    const auto parent = to_path(QStandardPaths::writableLocation(location));
    const auto folder = parent / LS_APP_VERSION_STR;

    std::filesystem::create_directory(folder);
    return std::filesystem::weakly_canonical(folder) / relative;
}

}  // namespace

auto get_writable_setting_path(setting_t settings) -> std::filesystem::path {
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
        case gui_settings: {
            return writable_standard_path(QStandardPaths::AppConfigLocation,
                                          "gui_settings.json");
        }
        case logfile: {
            return writable_standard_path(QStandardPaths::AppConfigLocation,
                                          "logging.txt");
        }
    };
    std::terminate();
}

}  // namespace logicsim
