#ifndef MANAGER_H
#define MANAGER_H

#include <QObject>
#include <QSettings>
#include <QDebug>
#include "chat.h"
#include "connection.h"
#include "chattabs.h"

class MainWindow;

class Manager : public QObject
{
Q_OBJECT
public:
    explicit Manager(QObject *parent = 0);
    ~Manager();

private slots:
    void sendNick();
    void connectToHost();
    void setNick(const QString &);
    void setHostname(const QString &);
    void setPort(const int &);
    void parseIncomingChatCommand();
    void parseOutgoingChatCommand(int chatID);
    void destroyTab(int);

private:
    Connection * _connection;
    QString _hostname;
    int _port;
    QSignalMapper *commandMapper;
    QSettings * _settings;

    QList<Chat*> chats;
    Chat * findChat(unsigned int chatID);
    Chat * initChat(ChatCommand &command);

    QTabWidget tabWidget;
    ChatTabs *tabMain;

    void readSettings();
};

#endif // MANAGER_H
