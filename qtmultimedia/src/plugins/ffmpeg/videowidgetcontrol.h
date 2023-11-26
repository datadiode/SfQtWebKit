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

#ifndef __VideoWidgetControl_H
#define __VideoWidgetControl_H

#include <QVideoWidgetControl>

class MediaPlayerControl;
class VideoWidget;

class VideoWidgetControl : public QVideoWidgetControl
{
    Q_OBJECT
private:
    bool                _fs;
    Qt::AspectRatioMode _am;
    int                 _brightness;
    int                 _contrast;
    int                 _hue;
    int                 _saturation;

    VideoWidget        *_video_widget;
    MediaPlayerControl *_ffmpeg;

public:
    explicit VideoWidgetControl(MediaPlayerControl* player, QObject* parent = nullptr);
    ~VideoWidgetControl() override;

    bool isFullScreen() const override;
    void setFullScreen(bool fullScreen) override;

    Qt::AspectRatioMode aspectRatioMode() const override;
    void setAspectRatioMode(Qt::AspectRatioMode mode) override;

    int brightness() const override { return _brightness; }
    void setBrightness(int brightness) override;

    int contrast() const override { return _contrast; }
    void setContrast(int contrast) override;

    int hue() const override { return _hue; }
    void setHue(int hue) override;

    int saturation() const override { return _saturation; }
    void setSaturation(int saturation) override;

    QWidget* videoWidget() override;
};

#endif
