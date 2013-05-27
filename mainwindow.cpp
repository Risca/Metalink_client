#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    Chat(parent),
    ui(new Ui::MainWindow)
#ifdef AUDIO
    ,
    audioStream(0)
#endif
{
    ui->setupUi(this);
    ui->nickView->setModel(&_participants);
    ui->messageView->setModel(&_messages);

    connect(ui->nickLineEdit, SIGNAL(textChanged(QString)), this, SIGNAL(nickChanged(QString)));
    connect(ui->hostnameLineEdit, SIGNAL(textChanged(QString)), this, SIGNAL(hostnameChanged(QString)));
    connect(ui->portBox, SIGNAL(valueChanged(int)), this, SIGNAL(portChanged(int)));

    connect(ui->connectButton, SIGNAL(clicked()), this, SIGNAL(connectToHost()));
    connect(ui->sendButton, SIGNAL(clicked()), this, SLOT(sendMessage()));
    connect(ui->plainTextEdit, SIGNAL(enterPressed()), this, SLOT(sendMessage()));
    connect(this,SIGNAL(dataChanged()),this,SLOT(scrollToBottom()));

    setWindowTitle(tr("Sandbox Client"));
    connect(ui->nickView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(startChat(QModelIndex)));

    readSettings();
}

MainWindow::~MainWindow()
{
    emit destroyed(this);
    foreach (ChatWindow *chat, chats) {
        delete chat;
    }

    delete ui;
}

void MainWindow::enableWidgets()
{
    ui->sendButton->setEnabled(true);
    ui->messageView->setEnabled(true);
    ui->plainTextEdit->setEnabled(true);
    ui->nickView->setEnabled(true);
}

void MainWindow::changeEvent(QEvent *e)
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

void MainWindow::startChat(QModelIndex index)
{
    QStringList with = index.data(Qt::DisplayRole).toStringList();

    _currentCommand.retranslateCommand(ChatCommand::Init, with);
    emit newCommand();
}

void MainWindow::sendMessage()
{
    QString message(ui->plainTextEdit->toPlainText());
    if (!message.isEmpty()) {
        _currentCommand.retranslateCommand(ChatCommand::Message, message);
        emit newCommand();
    }

    ui->plainTextEdit->clear();
}

void MainWindow::scrollToBottom()
{
    ui->messageView->scrollToBottom();
}

void MainWindow::readSettings()
{
    QSettings settings("Shadowrun Comlink","MetaLink");
    if (settings.contains("hostname")) {
        ui->hostnameLineEdit->setText(settings.value("hostname").toString());
    }
    if (settings.contains("port")) {
        ui->portBox->setValue(settings.value("port").toInt());
    }
    if (settings.contains("nick")) {
        ui->nickLineEdit->setText(settings.value("nick").toString());
    }

}

#ifdef AUDIO
void MainWindow::updateAudioStream(QString payload)
{
    if (payload == "none") {
        ui->actionAudioStream->setEnabled(false);
        ui->actionAudioStream->setChecked(false);
        if (audioStream) {
            audioStream->stop();
        }
    } else {
        if (audioStream == 0) {
            audioStream = new myAudioStream(payload);
        } else {
            audioStream->changePayload(payload);
        }
        ui->actionAudioStream->setEnabled(true);
        ui->actionAudioStream->setChecked(true);
    }

}

void MainWindow::on_actionAudioStream_toggled(bool mute)
{
    audioStream->muteAudio(!mute);
}
#endif
