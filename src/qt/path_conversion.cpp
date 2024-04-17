#include "qt/path_conversion.h"

#include "algorithm/u8_conversion.h"

#include <gsl/gsl>

namespace logicsim {

auto to_qt(const std::filesystem::path& path) -> QString {
    const auto u8string = path.u8string();
    return QString::fromUtf8(u8string.data(), gsl::narrow<qsizetype>(u8string.size()));
}

auto to_path(const QString& filename) -> std::filesystem::path {
    return std::filesystem::path {to_u8string(filename.toStdString())};
}

}  // namespace logicsim
