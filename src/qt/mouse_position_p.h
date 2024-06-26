#ifndef LOGICSIM_QT_MOUSE_POSITION_P_H
#define LOGICSIM_QT_MOUSE_POSITION_P_H

class QScreen;
class QPointF;

namespace logicsim {

/**
 * @brief: Get accurate global cursor position.
 *
 * This reimplements `QCursor::pos()` without rounding to int.
 * This method relies on Qt internals and might break. It works with Qt 6.7.0.
 *
 * Source: qtbase/src/gui/kernel/qcursor.cpp
 */
[[nodiscard]] auto cursor_position_high_dpi(const QScreen* screen) -> QPointF;
[[nodiscard]] auto cursor_position_high_dpi() -> QPointF;

// auto set_cursor_position_high_dpi(QScreen* screen, QPointF position) -> void;
// auto set_cursor_position_high_dpi(QPointF position) -> void;

}  // namespace logicsim

#endif
