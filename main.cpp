#include "direwolfconnect.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    DirewolfConnect w;
    w.show();
    return a.exec();
}
