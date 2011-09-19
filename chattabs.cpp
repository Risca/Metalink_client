#include "chattabs.h"

ChatTabs::ChatTabs(QWidget *parent) :
    QMainWindow(parent)
{
}

void ChatTabs::closeEvent(QCloseEvent *e)
{
    emit closed();
    e->accept();
}
