#include "file.h"

#include <QFile>
#include <QIODevice>
#include <QString>

namespace logicsim {

auto save_file(std::string filename, std::string binary) -> bool {
    auto file = QFile(QString::fromUtf8(filename));

    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    if (file.write(binary.data(), binary.size()) < 0) {
        return false;
    }

    return true;
}

auto load_file(std::string filename) -> std::string {
    auto file = QFile(QString::fromUtf8(filename));

    if (!file.open(QIODevice::ReadOnly)) {
        return "";
    }

    auto binary = std::string(file.size(), '\0');

    if (file.read(binary.data(), binary.size()) < 0) {
        return "";
    }

    return binary;
}

}  // namespace logicsim