
#include <QApplication>
#include <QFrame>
#include <QTextEdit>
#include <QVBoxLayout>

auto main(int argc, char* argv[]) -> int {
    // Note, QApplication needs to be created before any QMessageBox is shown.
    auto app [[maybe_unused]] = QApplication {argc, argv};

    auto frame = QFrame {};
    frame.setWindowTitle("Sample GUI");

    const auto layout = new QVBoxLayout {&frame};
    const auto edit = new QTextEdit {&frame};
    edit->setText("Hello World");
    layout->addWidget(edit);
    frame.resize(400, 400);

    frame.show();

    return QApplication::exec();
}
