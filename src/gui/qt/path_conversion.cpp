#include "gui/qt/path_conversion.h"

#include <gsl/gsl>

namespace logicsim {

auto to_qt(const std::filesystem::path& path) -> QString {
    // Qt and Windows filesystem::path on Windows uses UTF-16.
    // On Windows there will be no conversion
    return QString::fromStdU16String(path.u16string());
}

auto to_path(const QString& filename) -> std::filesystem::path {
    // Qt and Windows filesystem::path on Windows uses UTF-16.
    // On Windows there will be no conversion
    return std::filesystem::path {filename.toStdU16String()};
}

}  // namespace logicsim
