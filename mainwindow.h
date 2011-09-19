#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>

#include "chat.h"
#include "chatwindow.h"
#include "connection.h"
#ifdef AUDIO
#include "myaudiostream.h"
#endif

namespace Ui {
    class MainWindow;
}

class MainWindow : public Chat {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void enableWidgets();
#ifdef AUDIO
    void updateAudioStream(QString payload);
#endif

signals:
    void connectToHost();
    void nickChanged(QString);
    void hostnameChanged(QString);
    void portChanged(int);

protected:
    void changeEvent(QEvent *e);

private:
    Ui::MainWindow *ui;
    QList<ChatWindow*> chats;
    void readSettings();
#ifdef AUDIO
    myAudioStream *audioStream;
#endif

private slots:
#ifdef AUDIO
    void on_actionAudioStream_toggled(bool );
#endif
    void startChat(QModelIndex);
    void sendMessage();
    void scrollToBottom();
};

#endif // MAINWINDOW_H
