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

#ifndef __RenderControl_H
#define __RenderControl_H

#include <QMediaPlayerControl>
#include <QVideoRendererControl>

class MediaPlayerControl;
class QOpenGLFramebufferObject;

typedef int MediaStatus;

class RendererControl : public QVideoRendererControl
{
    Q_OBJECT
public:
    RendererControl(MediaPlayerControl* player, QObject *parent = nullptr);
    QAbstractVideoSurface *surface() const override;
    void setSurface(QAbstractVideoSurface *surface) override;

    void setSource();

public slots:
    void onFrameAvailable();
    void onMediaStateChanged(QMediaPlayer::MediaStatus status);

private:
    QAbstractVideoSurface    *_surface;
    MediaPlayerControl       *_ffmpeg;
    QOpenGLFramebufferObject *_fbo;

    // video_w/h_ is from MediaInfo, which may be incorrect.
    // A better value is from VideoFrame but it's not a public class now.

    int video_w_;
    int video_h_;
};

#endif
