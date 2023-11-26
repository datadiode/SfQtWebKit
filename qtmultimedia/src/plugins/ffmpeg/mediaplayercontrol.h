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

#ifndef __MediaPlayeControl__H
#define __MediaPlayeControl__H

#include <QMediaPlayerControl>
#include <QSize>
#include "ffmpegprovider.h"

class MediaPlayerControl : public QMediaPlayerControl
{
    Q_OBJECT
public:
    explicit MediaPlayerControl(QObject* parent = nullptr);

private:
    FFmpegProvider *_provider;

private:
    bool    _has_audio;
    bool    _has_video;
    bool    _muted;
    int     _volume;
    qint64  _duration;

public:
    QMediaPlayer::State state() const override;
    QMediaPlayer::MediaStatus mediaStatus() const override;

    qint64 duration() const override;
    qint64 position() const override;
    void setPosition(qint64 position) override;

    int volume() const override;
    void setVolume(int volume) override;

    bool isMuted() const override;
    void setMuted(bool muted) override;

    int bufferStatus() const override;

    bool isAudioAvailable() const override;
    bool isVideoAvailable() const override;

    bool isSeekable() const override;
    QMediaTimeRange availablePlaybackRanges() const override;

    qreal playbackRate() const override;
    void setPlaybackRate(qreal rate) override;

    QMediaContent media() const override;
    const QIODevice *mediaStream() const override;
    void setMedia(const QMediaContent &media, QIODevice *stream) override;

    void play() override;
    void pause() override;
    void stop() override;

public:
    void onStateChange(FFmpegProvider::State s);
    void onMediaStateChange(FFmpegProvider::MediaState s);
    void onEvent(const FFmpegProvider::MediaEvent &e);
    void onRender();

public:
    FFmpegProvider *provider();

signals:
    void frameAvailable();

};

#endif
