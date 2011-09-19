#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include "chat.h"

namespace Ui {
    class ChatWindow;
}

class ChatWindow : public Chat {
    Q_OBJECT
public:
    ChatWindow(const QStringList &participants, unsigned int chatID, QWidget *parent = 0);
    ~ChatWindow();
    void ok();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::ChatWindow *ui;

private slots:
    void sendMessage();
    void inviteNewParticipant();
};

#endif // CHATWINDOW_H
