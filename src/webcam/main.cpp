#include "webcam.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    webcam window;
    window.show();
    return app.exec();
}
