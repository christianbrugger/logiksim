

#include "main_widget.h"

#include <QApplication>
#include <QtOpenGLWidgets/QOpenGLWidget>

/* Needs to be called before create QApplication. */
void disable_opengl_vsync() {
    QSurfaceFormat format;
    format.setSwapInterval(0);
    QSurfaceFormat::setDefaultFormat(format);
}

int main(int argc, char* argv[]) {
    disable_opengl_vsync();
    QApplication app(argc, argv);

    logicsim::MainWidget frame;
    frame.show();

    return QApplication::exec();
}
