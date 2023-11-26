/*
 * ffmpeg-plugin - a Qt MultiMedia plugin for playback of video/audio using
 * the ffmpeg library for decoding.
 *
 * Copyright (C) 2021 Hans Dijkema, License: LGPLv3
 * https://github.com/hdijkema/qtmultimedia-plugin-ffmpeg
 *
 * Derived from the work of Wang Bin
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 */

#include "mediaplayercontrol.h"
#include "ffmpegprovider.h"
#include <QDebug>

#define LINE_DEBUG qDebug() << __FUNCTION__ << __LINE__

static QMediaPlayer::State toQt(FFmpegProvider::State value) {
    switch (value) {
        case FFmpegProvider::Playing: return QMediaPlayer::PlayingState;
        case FFmpegProvider::Paused: return QMediaPlayer::PausedState;
        case FFmpegProvider::Stopped: return QMediaPlayer::StoppedState;
    }
    return QMediaPlayer::StoppedState;
}

static QMediaPlayer::MediaStatus toQt(FFmpegProvider::MediaState value) {
    switch (value) {
        case FFmpegProvider::NoMedia: return QMediaPlayer::NoMedia;
        case FFmpegProvider::Invalid: return QMediaPlayer::InvalidMedia;
        default: break;
    }
    if (value & FFmpegProvider::Loading)
        return QMediaPlayer::LoadingMedia;
    if (value & FFmpegProvider::Stalled)
        return QMediaPlayer::StalledMedia;
    if (value & FFmpegProvider::Buffering)
        return QMediaPlayer::BufferingMedia;
    if (value & FFmpegProvider::Buffered)
        return QMediaPlayer::BufferedMedia; // playing or paused
    if (value & FFmpegProvider::End)
        return QMediaPlayer::EndOfMedia; // playback complete
    if (value & FFmpegProvider::Loaded)
        return QMediaPlayer::LoadedMedia; // connected, stopped. so last
    return QMediaPlayer::UnknownMediaStatus;
}



MediaPlayerControl::MediaPlayerControl(QObject* parent)
    : QMediaPlayerControl(parent)
    , _has_audio(true)
    , _has_video(true)
    , _muted(false)
    , _volume(100)
    , _duration(0)
{
    LINE_DEBUG;

    _provider = new FFmpegProvider(this);

    LINE_DEBUG;
}

QMediaPlayer::State MediaPlayerControl::state() const
{
    return toQt(_provider->state());
}

QMediaPlayer::MediaStatus MediaPlayerControl::mediaStatus() const
{
    return toQt(_provider->mediaState());
}

qint64 MediaPlayerControl::duration() const
{
    return _duration;
}

qint64 MediaPlayerControl::position() const
{
    return _provider->position();
}

void MediaPlayerControl::setPosition(qint64 position)
{
    _provider->seek(position);
}

int MediaPlayerControl::volume() const
{
    return _volume;
}

void MediaPlayerControl::setVolume(int volume)
{
    _volume = volume;
    _provider->setVolume(volume);
}

bool MediaPlayerControl::isMuted() const
{
    return _muted;
}

void MediaPlayerControl::setMuted(bool muted)
{
    _muted = muted;
    _provider->setMuted(muted);
}

int MediaPlayerControl::bufferStatus() const
{
    return 0;
}

bool MediaPlayerControl::isAudioAvailable() const
{
    return _has_audio;
}

bool MediaPlayerControl::isVideoAvailable() const
{
    return _has_video;
}

bool MediaPlayerControl::isSeekable() const
{
    return true;
}

QMediaTimeRange MediaPlayerControl::availablePlaybackRanges() const
{
    return QMediaTimeRange();
}

qreal MediaPlayerControl::playbackRate() const
{
    return _provider->playbackRate();
}

void MediaPlayerControl::setPlaybackRate(qreal rate)
{
    const auto old = playbackRate();

    if (playbackRate() == rate)
        return;

    _provider->setPlaybackRate(rate);

    if (playbackRate() != old) // success
        Q_EMIT playbackRateChanged(playbackRate());
}

QMediaContent MediaPlayerControl::media() const
{
    return QMediaContent();
}

const QIODevice* MediaPlayerControl::mediaStream() const
{
    return nullptr;
}

void MediaPlayerControl::setMedia(const QMediaContent& media, QIODevice* io)
{
    stop();
    if (io) {
        _provider->setMedia(QString("qio:%1").arg(qintptr(io)));
    } else {
        QUrl u(media.canonicalUrl());
        if (u.isLocalFile())
            _provider->setMedia(u.toLocalFile()); // for windows
        else
            _provider->setMedia(u.toString());
    }

    emit positionChanged(0);

    _provider->waitFor(FFmpegProvider::Stopped);

    const auto& info = _provider->mediaInfo();

    _duration = info.duration;
    _has_audio = info.has_audio;
    _has_video = info.has_video;

    emit durationChanged(_duration);
    emit audioAvailableChanged(_has_audio);
    emit videoAvailableChanged(_has_video);
    emit seekableChanged(false);
}

void MediaPlayerControl::play()
{
    _provider->setState(FFmpegProvider::Playing);
}

void MediaPlayerControl::pause()
{
    _provider->setState(FFmpegProvider::Paused);
}

void MediaPlayerControl::stop()
{
    _provider->setState(FFmpegProvider::Stopped);
}

void MediaPlayerControl::onStateChange(FFmpegProvider::State s)
{
    emit stateChanged(toQt(s));
}

void MediaPlayerControl::onMediaStateChange(FFmpegProvider::MediaState s)
{
    emit mediaStatusChanged(toQt(s));
}

void MediaPlayerControl::onEvent(const FFmpegProvider::MediaEvent &e)
{
    if (e.error < 0) {
        if (e.kind == FFmpegProvider::Audio ||
                e.kind == FFmpegProvider::Video) {
            emit error(QMediaPlayer::FormatError, tr("Unsupported media, a codec is missing."));
        }
    }
}

void MediaPlayerControl::onRender()
{
    emit frameAvailable();
}

FFmpegProvider *MediaPlayerControl::provider()
{
    return _provider;
}
