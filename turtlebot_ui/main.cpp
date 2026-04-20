#include "widget.h"
#include <QApplication>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication::addLibraryPath("/usr/lib/x86_64-linux-gnu/qt5/plugins");

    QApplication a(argc, argv);

    Widget w;
    w.show();
    return a.exec();
}
