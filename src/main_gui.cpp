

#include <QApplication>
#include <QFrame>
#include <QtOpenGLWidgets/QOpenGLWidget>


/* Needs to be called before create QApplication. */
void disable_opengl_vsync() {
    QSurfaceFormat format;
    format.setSwapInterval(0);
    QSurfaceFormat::setDefaultFormat(format);
}


int main(int argc, char* argv[])
{
    disable_opengl_vsync();
    QApplication app(argc, argv);

    QFrame frame;
    frame.show();

    return QApplication::exec();
}
