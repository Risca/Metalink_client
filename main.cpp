#include <QtGui/QApplication>
#include "manager.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Manager m;
    return a.exec();
}
