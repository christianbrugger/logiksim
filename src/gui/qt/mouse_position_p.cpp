#include "gui/qt/mouse_position_p.h"

#include <QPoint>
#include <QPointF>
#include <QScreen>

#include <private/qguiapplication_p.h>
#include <private/qhighdpiscaling_p.h>
#include <qpa/qplatformcursor.h>
#include <qpa/qplatformscreen.h>

namespace logicsim {

/**
 * Source: qtbase/src/gui/kernel/qcursor.cpp
 *
 * Change: Cast to `QPointF` before QHighDPI conversion.
 * Change: Remove `.toPoint()` on lastCursorPosition.
 *
 * Git:
 * https://github.com/qt/qtbase/blob/e7362764d4931f255d2377462df8ac7a0d4e7c84/src/gui/kernel/qcursor.cpp#L157-L168
 */
[[nodiscard]] auto cursor_position_high_dpi(const QScreen* screen) -> QPointF {
    if (screen) {
        if (const QPlatformCursor* cursor = screen->handle()->cursor()) {
            const QPlatformScreen* ps = screen->handle();
            QPoint nativePos = cursor->pos();
            ps = ps->screenForPosition(nativePos);
            return QHighDpi::fromNativePixels(QPointF {nativePos}, ps->screen());
        }
    }
    return QPointF {QGuiApplicationPrivate::lastCursorPosition};
}

[[nodiscard]] auto cursor_position_high_dpi() -> QPointF {
    return cursor_position_high_dpi(QGuiApplication::primaryScreen());
}

/*
auto set_cursor_position_high_dpi(QScreen* screen, QPointF position) -> void {
    if (screen) {
        if (QPlatformCursor* cursor = screen->handle()->cursor()) {
            const QPoint devicePos =
                QHighDpi::toNativePixels(position,
                                         screen->virtualSiblingAt(position.toPoint()))
                    .toPoint();

            // Need to check, since some X servers generate null mouse move
            // events, causing looping in applications which call setPos() on
            // every mouse move event.
            if (devicePos != cursor->pos()) {
                cursor->setPos(devicePos);
            };
        }
    }
}

auto set_cursor_position_high_dpi(QPointF position) -> void {
    set_cursor_position_high_dpi(QGuiApplication::primaryScreen(), position);
}
*/

}  // namespace logicsim
