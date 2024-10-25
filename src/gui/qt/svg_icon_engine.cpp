#include "gui/qt/svg_icon_engine.h"

#include "gui/format/qt_type.h"

#include "core/logging.h"

#include <gsl/gsl>

#include <QPainter>

namespace logicsim {

SvgIconEngine::SvgIconEngine(const std::string &svg_text) {
    svg_data_ = QByteArray::fromStdString(svg_text);
}

void SvgIconEngine::paint(QPainter *painter, const QRect &rect,
                          QIcon::Mode mode [[maybe_unused]],
                          QIcon::State state [[maybe_unused]]) {
    auto renderer = QSvgRenderer {svg_data_};
    renderer.render(painter, rect);
}

auto SvgIconEngine::clone() const -> QIconEngine * {
    return new SvgIconEngine(*this);
}

auto SvgIconEngine::pixmap(const QSize &size, QIcon::Mode mode,
                           QIcon::State state) -> QPixmap {
    // create empty pixmap with alpha channel
    // default implementation does not create alpha channel

    QImage img(size, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPixmap pm = QPixmap::fromImage(img, Qt::NoFormatConversion);
    {
        QPainter painter(&pm);
        this->paint(&painter, QRect {QPoint {}, size}, mode, state);
    }
    return pm;
}

}  // namespace logicsim
