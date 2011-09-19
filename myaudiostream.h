#ifndef MYAUDIOSTREAM_H
#define MYAUDIOSTREAM_H

#include <mediastreamer2/mediastream.h>
#include <mediastreamer2/msrtp.h>
#include <ortp/ortp.h>
#include <QString>
#include <QMessageBox>
#include <QInputDialog>
#include <QMap>
#include <QSettings>

class myAudioStream
{
public:
    explicit myAudioStream(QString payload);
    ~myAudioStream();
    void changePayload(const QString&);
    void stop();
    void muteAudio(bool);

private:
    static bool initialized;
    AudioStream *stream;
    MSSndCard *playcard;
    MSTicker *ticker;
    bool filtersLinked;

    bool init_card();
    bool init_stream();
    bool init_filters(const QString & payload);
    bool link_filters();
    bool start_ticker();

    bool stop_ticker();
    bool unlink_filters();
    void destroy_filters();
};

#endif // MYAUDIOSTREAM_H
