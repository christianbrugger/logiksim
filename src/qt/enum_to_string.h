#ifndef LOGICSIM_QT_ENUM_TO_STRING_H
#define LOGICSIM_QT_ENUM_TO_STRING_H

#include <QMetaEnum>

#include <string_view>

namespace logicsim {

template <typename QEnum>
auto qt_enum_to_string(const QEnum &value) -> std::string_view {
    return std::string_view {QMetaEnum::fromType<QEnum>().valueToKey(value)};
}

}  // namespace logicsim

#endif
