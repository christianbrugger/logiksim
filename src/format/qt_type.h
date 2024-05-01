#ifndef LOGICSIM_FORMAT_QT_TYPE_H
#define LOGICSIM_FORMAT_QT_TYPE_H

#include <fmt/core.h>

#include <QPoint>
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

//
// QPoint
//

template <typename Char>
struct fmt::formatter<QPoint, Char> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const QPoint &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "({}, {})", obj.x(), obj.y());
    }
};

//
// QPointF
//

template <typename Char>
struct fmt::formatter<QPointF, Char> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const QPointF &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "({:.3f}, {:.3f})", obj.x(), obj.y());
    }
};

#endif
