#include "file.h"

#include "algorithm/path_conversion.h"

#include <QFile>
#include <QIODevice>
#include <QString>

#include <filesystem>
#include <fstream>

namespace logicsim {

auto save_file(const QString &filename, const std::string &binary) -> bool {
    return save_file(filename.toStdString(), binary);
}

auto save_file(const std::string &filename, const std::string &binary) -> bool {
    const auto path = to_path(filename);

    auto file = std::ofstream(path, std::ios::out | std::ios::binary | std::ios::trunc);
    file << binary;
    return file.good();
}

auto load_file(const QString &filename) -> std::string {
    return load_file(filename.toStdString());
}

auto load_file(const std::string &filename) -> std::string {
    const auto path = to_path(filename);

    auto file = std::ifstream {path, std::ios::in | std::ios::binary};
    auto buffer = std::ostringstream {};
    buffer << file.rdbuf();

    if (!file.good()) {
        return "";
    }

    return buffer.str();
}

}  // namespace logicsim