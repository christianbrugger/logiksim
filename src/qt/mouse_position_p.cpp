#include "qt/mouse_position_p.h"

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
[[nodiscard]] auto cursor_position_p(const QScreen* screen) -> QPointF {
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

[[nodiscard]] auto cursor_position_p() -> QPointF {
    return cursor_position_p(QGuiApplication::primaryScreen());
}

}  // namespace logicsim
