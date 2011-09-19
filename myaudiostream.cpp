#include "myaudiostream.h"

bool myAudioStream::initialized = false;

myAudioStream::myAudioStream(QString payload)
{
    if (!initialized) {
        ortp_set_log_level_mask(ORTP_DEBUG|ORTP_ERROR);
        ms_init();
        ortp_init();
        initialized = true;
    }

    if (!init_card()) {
        return;
    }
    if (!init_stream()) {
        return;
    }
    if (!init_filters(payload)) {
        return;
    }
    if (!link_filters()) {
        return;
    }
    if (!start_ticker()) {
        return;
    }
}

myAudioStream::~myAudioStream()
{
    stop_ticker();
    unlink_filters();
    /* Destroy RTP session */
    ms_message("Destroying RTP session");
    rtp_session_destroy(stream->session);

    ortp_exit();
    ms_exit();
}

void myAudioStream::changePayload(const QString & payload)
{
    stop_ticker();
    unlink_filters();
    destroy_filters();

    init_filters(payload);
    link_filters();
    start_ticker();
}

void myAudioStream::stop()
{
    stop_ticker();
    unlink_filters();
    destroy_filters();
}

void myAudioStream::muteAudio(bool mute)
{
    ms_error("%s", mute ? "Muting sound" : "Un-muting sound");
}

bool myAudioStream::init_card()
{
    QSettings settings("Shadowrun Comlink","MetaLink");
    QString card_id;
    if (settings.contains("Audio playback card")) {
        card_id = settings.value("Audio playback card").toString();
    } else {
        const MSList *list = ms_snd_card_manager_get_list(ms_snd_card_manager_get());
        QStringList cards;
        while (list != NULL) {
            cards << ms_snd_card_get_string_id((MSSndCard*)list->data);
            list = list->next;
        }
        if (cards.isEmpty()) {
            QMessageBox::information(0, QObject::tr("No cards detected!"), QObject::tr("No cards detected!"));
            return false;
        }

        bool ok;
        card_id = QInputDialog::getItem(0, QObject::tr("Select playback card"),
                                                QObject::tr("Select playback card"),
                                                cards, 0, false, &ok);
        if (!ok) {
            return false;
        }
        settings.setValue("Audio playback card",card_id);
    }

    playcard = ms_snd_card_manager_get_card(ms_snd_card_manager_get(), card_id.toStdString().c_str());
    if (playcard==NULL){
        ms_error("No playback card found.");
        return false;
    } else {
        ms_message("Got card: %s", ms_snd_card_get_string_id(playcard));
    }

    return true;
}

bool myAudioStream::init_stream()
{
    /** Init stream **/
    stream = (AudioStream *)ms_new0(AudioStream,1);
    if (stream == 0) {
        ms_error("Failed to create new stream");
        return false;
    }

    /** Configure stream **/
    stream->play_dtmfs = false;
    stream->use_gc = false;
    stream->use_agc = false;
    stream->use_ng = false;

    /** Init RTP session **/
    stream->session = rtp_session_new(RTP_SESSION_RECVONLY);
    if (stream->session == 0) {
        ms_error("Failed to create new RTP session");
        return false;
    }

    /** Configure RTP session **/
    /* Create profile to use in session */
    RtpProfile *rtp_profile = rtp_profile_new("My profile");
    if (rtp_profile == 0) {
        ms_error("Failed to create new RTP profile");
        return false;
    }
    rtp_session_set_profile(stream->session, rtp_profile);

    /* Define some payloads */
    rtp_profile_set_payload(rtp_profile,110,&payload_type_speex_nb);
    rtp_profile_set_payload(rtp_profile,111,&payload_type_speex_wb);
    rtp_profile_set_payload(rtp_profile,112,&payload_type_speex_uwb);

    /* Set local address and port */
    rtp_session_set_local_addr(stream->session, "0.0.0.0", 1337);

    return true;
}

bool myAudioStream::init_filters(const QString & payload)
{
    /** Init filters **/
    stream->soundwrite = ms_snd_card_create_writer(playcard);
    RtpProfile *profile = rtp_session_get_profile(stream->session);
    PayloadType *pt;

    /* List all available payloads */
    QMap<QString,int> payloads;
    for (int i = 0; i < RTP_PROFILE_MAX_PAYLOADS; i++) {
        pt = rtp_profile_get_payload(profile,i);
        if (pt != 0) {
            QString payload(pt->mime_type);
            if (payloads.contains(payload)) {
                payload.append(" " + QString::number(pt->clock_rate));
            }
            payloads.insert(payload,i);
        }
    }

    if (!payloads.contains(payload)) {
        ms_error("Could not find payload %s", payload.toStdString().c_str());
        return false;
    }

    int payload_type_number = payloads.value(payload);
    /* Create filters */
    pt = rtp_profile_get_payload(profile,payload_type_number);
    stream->decoder = ms_filter_create_decoder(pt->mime_type);
    stream->rtprecv = ms_filter_new(MS_RTP_RECV_ID);
    stream->dtmfgen = ms_filter_new(MS_DTMF_GEN_ID);

    /** Configure filter options **/
    /* Set payload type to use when receiving */
    rtp_session_set_payload_type(stream->session, payload_type_number);
    /* Set session used by rtprecv */
    ms_filter_call_method(stream->rtprecv,MS_RTP_RECV_SET_SESSION,stream->session);
    /* Setup soundwrite and decoder parameters */
    int sr = pt->clock_rate;
    int chan = pt->channels;
    if (ms_filter_call_method(stream->soundwrite, MS_FILTER_SET_SAMPLE_RATE, &sr) !=0 ) {
        ms_error("Problem setting sample rate on soundwrite filter!");
        return false;
    }
    if (ms_filter_call_method(stream->soundwrite, MS_FILTER_SET_NCHANNELS, &chan) != 0) {
        ms_error("Failed to set sample rate on soundwrite filter!");
        return false;
    }

    if (ms_filter_call_method(stream->decoder, MS_FILTER_SET_SAMPLE_RATE, &sr) != 0) {
        ms_error("Problem setting sample rate on decoder filter!");
        return false;
    }
    return true;
}

bool myAudioStream::link_filters()
{
    /** Create graph (rtprecv --> decoder --> dtmfgen --> soundwrite) **/
    /* Link filters together */
    if (ms_filter_link(stream->rtprecv,0,stream->decoder,0) != 0) {
        ms_error("Failed to link rtprecv --> decoder");
        return false;
    }
    if (ms_filter_link(stream->decoder,0,stream->dtmfgen,0) != 0) {
        ms_error("Failed to link decoder --> dtmfgen");
        return false;
    }
    if (ms_filter_link(stream->dtmfgen,0,stream->soundwrite,0) != 0) {
        ms_error("Failed to link dtmfgen --> soundwrite");
        return false;
    }
    filtersLinked = true;
    return true;
}

bool myAudioStream::start_ticker()
{
    /* Create ticker */
    ticker = ms_ticker_new();
    if (ticker == 0) {
        ms_error("Failed to create new ticker");
        return false;
    }
    /* Attach chain to ticker */
    if (ms_ticker_attach(ticker, stream->rtprecv) != 0) {
        ms_error("Failed to attach rtprecv to ticker");
        return false;
    }
    return true;
}

bool myAudioStream::stop_ticker()
{
    ms_message("Stopping ticker");
    /* Detach ticker */
    if (ticker) {
        ms_message("Detaching ticker");
        if (ms_ticker_detach(ticker, stream->soundwrite) != 0) {
            ms_error("Failed to detach ticker");
            return false;
        }
    } else {
        ms_warning("No ticker to detach");
    }

    return true;
}

bool myAudioStream::unlink_filters()
{
    /* Unlink filters */
    if (filtersLinked) {
        ms_message("Unlinking filters");
        if (ms_filter_unlink(stream->dtmfgen,0,stream->soundwrite,0) != 0) {
            ms_error("Failed to unlink dtmfgen --> soundwrite");
            return false;
        }
        if (ms_filter_unlink(stream->decoder,0,stream->dtmfgen,0) != 0) {
            ms_error("Failed to unlink decoder --> dtmfgen");
            return false;
        }
        if (ms_filter_unlink(stream->rtprecv,0,stream->decoder,0) != 0) {
            ms_error("Failed to unlink rtprecv --> decoder");
            return false;
        }
        filtersLinked = false;
    }

    return true;
}

void myAudioStream::destroy_filters()
{
    ms_message("Destroying filters");
    if (stream->rtpsend != NULL) ms_filter_destroy(stream->rtpsend);
    if (stream->soundread != NULL) ms_filter_destroy(stream->soundread);
    if (stream->encoder != NULL) ms_filter_destroy(stream->encoder);
}
