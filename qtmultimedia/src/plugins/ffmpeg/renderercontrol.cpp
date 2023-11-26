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

#include "renderercontrol.h"
#include "mediaplayercontrol.h"

#include <QAbstractVideoBuffer>
#include <QAbstractVideoSurface>
#include <QVideoFrame>
#include <QVideoSurfaceFormat>
#include <QImage>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <QDebug>
#include <QOpenGLPaintDevice>
#include <QPainter>

#define LINE_DEBUG qDebug() << __FUNCTION__ << __LINE__

inline bool flags_added(unsigned a, unsigned b, unsigned fl)
{
    return (b & fl) > (a & fl);
}

RendererControl::RendererControl(MediaPlayerControl* player, QObject* parent)
    : QVideoRendererControl(parent)
    , _ffmpeg(player)
    , _surface(nullptr)
    , _fbo(nullptr)
    , video_w_(0)
    , video_h_(0)

{
    connect(_ffmpeg, &MediaPlayerControl::frameAvailable, this, &RendererControl::onFrameAvailable);
    connect(_ffmpeg, &MediaPlayerControl::mediaStatusChanged, this, &RendererControl::onMediaStateChanged);
}

QAbstractVideoSurface* RendererControl::surface() const
{
    return _surface;
}

void RendererControl::setSurface(QAbstractVideoSurface* surface)
{
    FFmpegProvider *provider = _ffmpeg->provider();

    if (_surface && _surface->isActive())
        _surface->stop();

    _surface = surface;

    if (!surface) {
        //provider->setRenderCallback(nullptr); // surfcace is set to null before destroy, avoid invokeMethod() on invalid this
        return;
    }

    const QSize r = surface->nativeResolution(); // may be (-1, -1)
    // mdk player needs a vo. add before delivering a video frame

    if (provider->mediaInfo().has_video) {
        auto &c = provider->mediaInfo().video;
        video_w_ = c.width;
        video_h_ = c.height;
    }

    if (r.width() < 0 || r.height() < 0) {
        provider->setVideoSurfaceSize(video_w_, video_h_);
    } else {
        provider->setVideoSurfaceSize(r.width(), r.height());
    }
}

void RendererControl::onMediaStateChanged(QMediaPlayer::MediaStatus status)
{
    FFmpegProvider *provider = this->_ffmpeg->provider();
    if (status == QMediaPlayer::LoadedMedia) {
        if (provider->mediaInfo().has_video) {
            auto &c = provider->mediaInfo().video;
            video_w_ = c.width;
            video_h_ = c.height;
        }
    }
}

void RendererControl::onFrameAvailable()
{
    if (!_surface)
        return;

    if (video_w_ <= 0 || video_h_ <= 0)
        return; // not playing, e.g. when stop() is called there is also a frameAvailable
                // signal to update renderer which is required by mdk internally.
                // if create fbo with an invalid size anyway, qt gl rendering will be broken forever

    FFmpegProvider *provider = _ffmpeg->provider();
    bool gotIt;
    QImage *img = provider->getImage(gotIt);

    if (gotIt) {
        QVideoFrame frame(*img);

        if (!_surface->isActive()) { // || surfaceFormat()!=
            QVideoSurfaceFormat format(QSize(video_w_, video_h_), QVideoFrame::Format_RGB32, QAbstractVideoBuffer::NoHandle);
            _surface->start(format);
        }

        _surface->present(frame); // main thread

        provider->popImage();
    }
}
