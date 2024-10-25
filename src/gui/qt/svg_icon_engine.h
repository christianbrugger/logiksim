#ifndef LOGICSIM_GUI_QT_SVG_ICON_ENGINE_H
#define LOGICSIM_GUI_QT_SVG_ICON_ENGINE_H

#include <QIconEngine>
#include <QSvgRenderer>

namespace logicsim {

/**
 * @brief: Render engine for Svg Icons
 *
 * Usage:
 *     QIcon {new SvgIconEngine {svg_text}}
 */
class SvgIconEngine : public QIconEngine {
   public:
    explicit SvgIconEngine() = default;
    explicit SvgIconEngine(const std::string &svg_text);

    auto paint(QPainter *painter, const QRect &rect, QIcon::Mode mode,
               QIcon::State state) -> void override;

    [[nodiscard]] auto clone() const -> QIconEngine * override;

    auto pixmap(const QSize &size, QIcon::Mode mode,
                QIcon::State state) -> QPixmap override;

   private:
    QByteArray svg_data_ {};
};

}  // namespace logicsim

#endif
