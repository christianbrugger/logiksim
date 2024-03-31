#ifndef LOGICSIM_QT_PATH_CONVERSION_H
#define LOGICSIM_QT_PATH_CONVERSION_H

#include <QString>

#include <filesystem>

namespace logicsim {

[[nodiscard]] auto to_qt(const std::filesystem::path& path) -> QString;

[[nodiscard]] auto to_path(const QString& filename) -> std::filesystem::path;

}  // namespace logicsim

#endif
