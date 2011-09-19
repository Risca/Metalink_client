#include "manager.h"
#include "mainwindow.h"

Manager::Manager(QObject *parent) :
    QObject(parent),
    _connection(new Connection(this)),
    _hostname("localhost"),
    _port(1337),
    commandMapper(new QSignalMapper(this)),
    _settings(new QSettings("Shadowrun Comlink","MetaLink",this))
{
    MainWindow *main = new MainWindow();
    connect(main, SIGNAL(nickChanged(QString)), this, SLOT(setNick(QString)));
    connect(main, SIGNAL(hostnameChanged(QString)), this, SLOT(setHostname(QString)));
    connect(main, SIGNAL(portChanged(int)), this, SLOT(setPort(int)));
    connect(main, SIGNAL(connectToHost()), this, SLOT(connectToHost()));

    connect(commandMapper, SIGNAL(mapped(int)), this, SLOT(parseOutgoingChatCommand(int)));
    connect(main, SIGNAL(newCommand()), commandMapper, SLOT(map()));
    commandMapper->setMapping(main, main->id());

    chats.append(main);
    main->show();

    readSettings();

    connect(_connection, SIGNAL(connected()), this, SLOT(sendNick()));
    connect(_connection, SIGNAL(connected()), main, SLOT(enableWidgets()));
    connect(_connection, SIGNAL(newChatCommand()), this, SLOT(parseIncomingChatCommand()));
#ifdef AUDIO
    connect(_connection, SIGNAL(audioAvailableUsingPayload(QString)),
            main, SLOT(updateAudioStream(QString)));
#endif
    tabWidget.setTabsClosable(true);
    connect(&tabWidget,SIGNAL(tabCloseRequested(int)),SLOT(destroyTab(int)));
    tabMain.setCentralWidget(&tabWidget);
}

void Manager::sendNick()
{
    _connection->send(Connection::Nick, _connection->nick());
}

void Manager::connectToHost()
{
    _connection->connectToHost(_hostname, _port);
}

void Manager::setNick(const QString & nick)
{
    _connection->setNick(nick);
    _settings->setValue("nick",nick);
}

void Manager::setHostname(const QString & hostname)
{
    _hostname = hostname;
    _settings->setValue("hostname",hostname);
}

void Manager::setPort(const int & port)
{
    _port = port;
    _settings->setValue("port",port);
}

void Manager::parseIncomingChatCommand()
{
    qDebug() << "Parsing incoming ChatCommand...";
    ChatCommand command = _connection->currentCommand();
    Chat *chat = findChat(command.chatID());

    switch (command.type()) {
    case ChatCommand::Invite:
        if (!chat) {
            qDebug() << "Got invited to chat! ^^";
            Chat * chat = initChat(command);
            if (chat) {
                // Create chat tab
                tabWidget.addTab(chat,chat->participants().join(", "));
                connect(&tabMain,SIGNAL(closed()),chat,SLOT(close()));
                tabMain.show();
                command.retranslateCommand(ChatCommand::Accept);
            } else {
                command.retranslateCommand(ChatCommand::Reject);
            }
            _connection->send(Connection::Chat,command.commandString());
        }
        break;
    case ChatCommand::List:
        if (chat) {
            qDebug() << "Got Chat list with:" << command.participants();
            chat->setParticipants(command.participants());
            tabWidget.setTabText(tabWidget.indexOf(chat),command.participants().join(", "));
        }
        break;
    case ChatCommand::Leave:
        qDebug() << "Kicked by server from Chat" << QString::number(chat->id());
        chat->hide();
        QMessageBox::information(chats.first(), tr("Kicked by DM"),
                                 tr("Kicked from chat:\n")+command.message());
        // Remove chat tab
        tabWidget.removeTab(tabWidget.indexOf(chat));
        // Close tabMain if there are no tabs left
        if (tabWidget.count()==0) {
            tabMain.hide();
        }
        chats.removeOne(chat);
        chat->deleteLater();
        break;
    case ChatCommand::Message:
        if (chat)
            qDebug() << "Message to Chat" << QString::number(command.chatID()) << ": " << command.participants();
            chat->appendMessage(command.participants().first(), command.message());
        break;
    default:
        // End of commands initiated from server
        break;
    }

    qDebug() << "Done parsing ChatCommand";
}

void Manager::parseOutgoingChatCommand(int chatID)
{
    qDebug() << "Parsing outgoing ChatCommand...";
    foreach(Chat * chat, chats) {
        if (*chat == chatID) { // Operator magic!
            // Found chat sending command!
            ChatCommand command = chat->command();
            switch (command.type()) {
            case ChatCommand::Init:
                {
                    QStringList participants = command.participants();
                    if (!participants.contains(_connection->nick())) {
                        participants.append(_connection->nick());
                    }
                    command.retranslateCommand(ChatCommand::Init, participants);
                }
                break;
            case ChatCommand::Invite:
                {
                    // Time to get some participants to this chat!
                    // TODO: Invite _multiple_ participants
                    bool ok;
                    QString foo = QInputDialog::getItem(0,QString("Select new participant"),
                                                        QString("Select new participants:"),
                                                        chats.first()->participants(),
                                                        0,false,&ok);
                    if (!ok || foo.isEmpty()) {
                        return;
                    }
                    command.retranslateCommand(ChatCommand::Invite,QStringList(foo));
                }
                break;
            case ChatCommand::Accept:
                break;
            case ChatCommand::Reject:
                break;
            case ChatCommand::Leave:
                chats.removeOne(chat);
                chat->deleteLater();
                break;
            case ChatCommand::Message:
                command.retranslateCommand(ChatCommand::Message,
                                           QStringList(_connection->nick()));
                break;
            default:
                break;
            }

            _connection->send(Connection::Chat, command.commandString());
            return;
        }
    }
    qDebug() << "Got newCommand() signal, but couldn't find chat!";
}

void Manager::destroyTab(int index)
{
    Chat * chat = (Chat*)tabWidget.widget(index);
    if (chat != 0) {
        chat->close();
        tabWidget.removeTab(index);
        if (tabWidget.count() < 1) {
            tabMain.hide();
        }
    }
}

Chat* Manager::findChat(unsigned int chatID)
{
    foreach (Chat *chat, chats) {
        if (*chat == chatID) {
            return chat;
        }
    }
    return 0;
}

Chat* Manager::initChat(ChatCommand &command)
{
    Chat * chat;
    QStringList participants = command.participants();
    if (participants.contains(_connection->nick())) {
        // I requested this Chat!
        chat = new ChatWindow(participants,command.chatID(),chats.first());
    } else {
        // New Chat. Do we want to participate?
        int ret = QMessageBox::question(chats.first(), tr("New chat"),
                                        tr("The following people wants to contact you:\n") +
                                        participants.join("\n"),
                                        QMessageBox::Ok, QMessageBox::Cancel);
        if (ret == QMessageBox::Ok) {
            participants.append(_connection->nick());
            chat = new ChatWindow(participants,command.chatID(),chats.first());
        }
    }

    if (chat) {
        connect(chat, SIGNAL(newCommand()), commandMapper, SLOT(map()));
        commandMapper->setMapping(chat, chat->id());

        chats.append(chat);
    }

    return chat;
}

void Manager::readSettings()
{
    if (_settings->contains("hostname")) {
        setHostname(_settings->value("hostname").toString());
    }

    if (_settings->contains("port")) {
        setPort(_settings->value("port").toInt());
    }

    if (_settings->contains("nick")) {
        _connection->setNick(_settings->value("nick").toString());
    } else {
        _connection->setNick("Risca");
        _settings->setValue("nick","Risca");
    }
}
