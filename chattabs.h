#ifndef CHATTABS_H
#define CHATTABS_H

#include <QMainWindow>
#include <QCloseEvent>

class ChatTabs : public QMainWindow
{
    Q_OBJECT
public:
    explicit ChatTabs(QWidget *parent = 0);

signals:
    void closed();

public slots:

protected:
    void closeEvent(QCloseEvent *);

};

#endif // CHATTABS_H
