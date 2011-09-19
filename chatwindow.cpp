#include "chatwindow.h"
#include "ui_chatwindow.h"
#include "mainwindow.h"

#include <QInputDialog>

ChatWindow::ChatWindow(const QStringList &participants, unsigned int chatID, QWidget *parent) :
    Chat(parent, chatID),
    ui(new Ui::ChatWindow)
{
    setParticipants(participants);
    ui->setupUi(this);
    ui->nickView->setModel(&_participants);
    ui->messageView->setModel(&_messages);
    connect(ui->plainTextEdit, SIGNAL(enterPressed()), this, SLOT(sendMessage()));
    connect(ui->sendButton, SIGNAL(clicked()), this, SLOT(sendMessage()));
    connect(this,SIGNAL(dataChanged()),ui->messageView,SLOT(scrollToBottom()));

    QAction *inviteParticipant = new QAction(QIcon(":/icons/plus-sign.jpg"),"Invite new participant",this);
    connect(inviteParticipant,SIGNAL(triggered()),this,SLOT(inviteNewParticipant()));
    ui->toolBar->addAction(inviteParticipant);
}

ChatWindow::~ChatWindow()
{
    delete ui;
}

void ChatWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void ChatWindow::sendMessage()
{
    QString message(ui->plainTextEdit->toPlainText());
    if (!message.isEmpty()) {
        _currentCommand.retranslateCommand(ChatCommand::Message, message);
        emit newCommand();
        ui->plainTextEdit->clear();
    }
}

void ChatWindow::inviteNewParticipant()
{
    // Let the manager handle it from here!
    _currentCommand.retranslateCommand(ChatCommand::Invite,QStringList());
    emit newCommand();
}
