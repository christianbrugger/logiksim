#ifndef LOGICSIM_FORMAT_QT_TYPE_H
#define LOGICSIM_FORMAT_QT_TYPE_H

#include <fmt/core.h>

#include <QString>

//
// QString
//

template <typename Char>
struct fmt::formatter<QString, Char> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const QString &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}", obj.toStdString());
    }
};

#endif
