# -------------------------------------------------
# Project created by QtCreator 2010-03-31T23:49:01
# -------------------------------------------------

#DEFINES += AUDIO
QT += network
TARGET = sandbox
TEMPLATE = app
INCLUDEPATH += ../Metalink_Common/ChatCommand \
    ../Metalink_Common/Chat \
    ../Metalink_Common/Connection \
    ../Metalink_Common/customPlainTextEdit
LIBS += -L../Metalink_Common-libs \
    -lChatCommand \
    -lChat \
    -lConnection \
    -lcustomplaintexteditplugin
SOURCES += main.cpp \
    mainwindow.cpp \
    chatwindow.cpp \
    manager.cpp \
    chattabs.cpp
HEADERS += mainwindow.h \
    chatwindow.h \
    manager.h \
    chattabs.h
FORMS += mainwindow.ui \
    chatwindow.ui

RESOURCES += \
    icons.qrc

# Custom audio stuff here
contains(DEFINES, AUDIO) {
    message(Building with audio enabled...)
    HEADERS += myaudiostream.h
    SOURCES += myaudiostream.cpp
    LIBS += -lmediastreamer -lortp
}
