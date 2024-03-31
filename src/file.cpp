#include "file.h"

#include "algorithm/path_conversion.h"

#include <QFile>
#include <QIODevice>
#include <QString>

#include <filesystem>
#include <fstream>

namespace logicsim {

namespace {

auto _save_file(const QString &filename_qt, const std::string &binary) -> bool {
    const auto path = to_path(filename_qt.toStdString());

    auto file = std::ofstream(path, std::ios::out | std::ios::binary | std::ios::trunc);
    file << binary;
    return file.good();
}

auto _load_file(const QString &filename_qt) -> std::string {
    const auto path = to_path(filename_qt.toStdString());

    auto file = std::ifstream {path, std::ios::in | std::ios::binary};
    auto buffer = std::ostringstream {};
    buffer << file.rdbuf();

    if (!file.good()) {
        return "";
    }

    return buffer.str();
}

}  // namespace

auto save_file(const QString &filename, const std::string &binary) -> bool {
    /*
    auto file = QFile(filename);

    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    if (file.write(binary.data(), binary.size()) < 0) {
        return false;
    }

    return true;
    */
    return _save_file(filename, binary);
}

auto save_file(const std::string &filename, const std::string &binary) -> bool {
    return save_file(QString::fromUtf8(filename), binary);
}

auto load_file(const QString &filename) -> std::string {
    /*
    auto file = QFile(filename);

    if (!file.open(QIODevice::ReadOnly)) {
        return "";
    }

    auto binary = std::string(file.size(), '\0');

    if (file.read(binary.data(), binary.size()) < 0) {
        return "";
    }

    return binary;
    */
    return _load_file(filename);
}

auto load_file(const std::string &filename) -> std::string {
    return load_file(QString::fromUtf8(filename));
}

}  // namespace logicsim