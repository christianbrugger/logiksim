#ifndef LOGICSIM_FORMAT_QT_TYPE_H
#define LOGICSIM_FORMAT_QT_TYPE_H

#include <fmt/core.h>

#include <QPoint>
#include <QRect>
#include <QSize>
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

template <typename Char>
struct fmt::formatter<QPointF, Char> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const QPointF &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "({:.3f}, {:.3f})", obj.x(), obj.y());
    }
};

//
// QRect
//

template <typename Char>
struct fmt::formatter<QRect, Char> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const QRect &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "QRect(x={}, y={}, w={}, h={})", obj.x(),
                              obj.y(), obj.width(), obj.height());
    }
};

template <typename Char>
struct fmt::formatter<QRectF, Char> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const QRectF &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "QRectF(x={:.3f}, y={:.3f}, w={:.3f}, h={:.3f})",
                              obj.x(), obj.y(), obj.width(), obj.height());
    }
};

//
// QSize
//

template <typename Char>
struct fmt::formatter<QSize, Char> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const QSize &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "QSize(w={}, h={})", obj.width(), obj.height());
    }
};

template <typename Char>
struct fmt::formatter<QSizeF, Char> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const QSizeF &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "QSizeF(w={:.3f}, h={:.3f})", obj.width(),
                              obj.height());
    }
};

#endif
