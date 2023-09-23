#include "file.h"

#include <QFile>
#include <QIODevice>
#include <QString>

namespace logicsim {

auto save_file(const QString &filename, const std::string &binary) -> bool {
    auto file = QFile(filename);

    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    if (file.write(binary.data(), binary.size()) < 0) {
        return false;
    }

    return true;
}

auto save_file(const std::string &filename, const std::string &binary) -> bool {
    return save_file(QString::fromUtf8(filename), binary);
}

auto load_file(const QString &filename) -> std::string {
    auto file = QFile(filename);

    if (!file.open(QIODevice::ReadOnly)) {
        return "";
    }

    auto binary = std::string(file.size(), '\0');

    if (file.read(binary.data(), binary.size()) < 0) {
        return "";
    }

    return binary;
}

auto load_file(const std::string &filename) -> std::string {
    return load_file(QString::fromUtf8(filename));
}

}  // namespace logicsim