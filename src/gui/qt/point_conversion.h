#ifndef LOGICSIM_QT_POINT_CONVERSION_H
#define LOGICSIM_QT_POINT_CONVERSION_H

#include "core/vocabulary/point_device.h"
#include "core/vocabulary/point_device_fine.h"
#include "core/vocabulary/size_device.h"

#include <QPoint>
#include <QPointF>
#include <QSize>

#include <concepts>
#include <utility>

namespace logicsim {

[[nodiscard]] constexpr auto to(QPoint point) -> point_device_t;
[[nodiscard]] constexpr auto to(QPointF point) -> point_device_fine_t;
[[nodiscard]] constexpr auto to(QSize size) -> size_device_t;

[[nodiscard]] constexpr auto from(point_device_t point) -> QPoint;
[[nodiscard]] constexpr auto from(point_device_fine_t point) -> QPointF;
[[nodiscard]] constexpr auto from(size_device_t size) -> QSize;

//
// Check types match
//

static_assert(std::same_as<decltype(std::declval<QPoint>().x()),
                           decltype(std::declval<point_device_t>().x)>);
static_assert(std::same_as<decltype(std::declval<QPoint>().y()),
                           decltype(std::declval<point_device_t>().y)>);

static_assert(std::same_as<decltype(std::declval<QPointF>().x()),
                           decltype(std::declval<point_device_fine_t>().x)>);
static_assert(std::same_as<decltype(std::declval<QPointF>().y()),
                           decltype(std::declval<point_device_fine_t>().y)>);

static_assert(std::same_as<decltype(std::declval<QSize>().width()),
                           decltype(std::declval<size_device_t>().width)>);
static_assert(std::same_as<decltype(std::declval<QSize>().height()),
                           decltype(std::declval<size_device_t>().height)>);

//
// Implementation
//

constexpr auto to(QPoint point) -> point_device_t {
    return point_device_t {point.x(), point.y()};
}

constexpr auto to(QPointF point) -> point_device_fine_t {
    return point_device_fine_t {point.x(), point.y()};
}

constexpr auto to(QSize size) -> size_device_t {
    return size_device_t {size.width(), size.height()};
}

constexpr auto from(point_device_t point) -> QPoint {
    return QPoint {point.x, point.y};
}

constexpr auto from(point_device_fine_t point) -> QPointF {
    return QPointF {point.x, point.y};
}

constexpr auto from(size_device_t size) -> QSize {
    return QSize {size.width, size.height};
}

}  // namespace logicsim

#endif
