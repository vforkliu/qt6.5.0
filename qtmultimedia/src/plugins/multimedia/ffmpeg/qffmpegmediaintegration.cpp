// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtMultimedia/private/qplatformmediaplugin_p.h>
#include <qcameradevice.h>
#include "qffmpegmediaintegration_p.h"
#include "qffmpegmediaformatinfo_p.h"
#include "qffmpegmediaplayer_p.h"
#include "qffmpegvideosink_p.h"
#include "qffmpegmediacapturesession_p.h"
#include "qffmpegmediarecorder_p.h"
#include "qffmpegimagecapture_p.h"
#include "qffmpegaudioinput_p.h"
#include "qffmpegaudiodecoder_p.h"
#include "qffmpegscreencapture_p.h"

#ifdef Q_OS_MACOS
#include <VideoToolbox/VideoToolbox.h>
#endif

#ifdef Q_OS_DARWIN
#include "qavfcamera_p.h"
#include "qavfscreencapture_p.h"
#elif defined(Q_OS_WINDOWS)
#include "qwindowscamera_p.h"
#include "qwindowsvideodevices_p.h"
#include "qffmpegscreencapture_dxgi_p.h"
#endif

#ifdef Q_OS_ANDROID
#    include "jni.h"
#    include "qandroidvideodevices_p.h"
#    include "qandroidcamera_p.h"
extern "C" {
#    include <libavcodec/jni.h>
}
#endif

#if QT_CONFIG(linux_v4l)
#include "qv4l2camera_p.h"
#endif

#if QT_CONFIG(cpp_winrt)
#include "qffmpegscreencapture_uwp_p.h"
#endif

#if QT_CONFIG(xlib)
#include "qx11screencapture_p.h"
#endif

QT_BEGIN_NAMESPACE

class QFFmpegMediaPlugin : public QPlatformMediaPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformMediaPlugin_iid FILE "ffmpeg.json")

public:
    QFFmpegMediaPlugin()
      : QPlatformMediaPlugin()
    {}

    QPlatformMediaIntegration* create(const QString &name) override
    {
        if (name == QLatin1String("ffmpeg"))
            return new QFFmpegMediaIntegration;
        return nullptr;
    }
};

QFFmpegMediaIntegration::QFFmpegMediaIntegration()
{
    m_formatsInfo = new QFFmpegMediaFormatInfo();

#if QT_CONFIG(linux_v4l)
    m_videoDevices = std::make_unique<QV4L2CameraDevices>(this);
#endif
#ifdef Q_OS_DARWIN
    m_videoDevices = std::make_unique<QAVFVideoDevices>(this);
#elif defined(Q_OS_ANDROID)
    m_videoDevices = std::make_unique<QAndroidVideoDevices>(this);
#elif defined(Q_OS_WINDOWS)
    m_videoDevices = std::make_unique<QWindowsVideoDevices>(this);
#endif

    if (qgetenv("QT_FFMPEG_DEBUG").toInt())
        av_log_set_level(AV_LOG_DEBUG);

#ifndef QT_NO_DEBUG
    qDebug() << "Available HW decoding frameworks:";
    AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
    while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
        qDebug() << "    " << av_hwdevice_get_type_name(type);
#endif
}

QFFmpegMediaIntegration::~QFFmpegMediaIntegration()
{
    delete m_formatsInfo;
}

QPlatformMediaFormatInfo *QFFmpegMediaIntegration::formatInfo()
{
    return m_formatsInfo;
}

QMaybe<QPlatformAudioDecoder *> QFFmpegMediaIntegration::createAudioDecoder(QAudioDecoder *decoder)
{
    return new QFFmpegAudioDecoder(decoder);
}

QMaybe<QPlatformMediaCaptureSession *> QFFmpegMediaIntegration::createCaptureSession()
{
    return new QFFmpegMediaCaptureSession();
}

QMaybe<QPlatformMediaPlayer *> QFFmpegMediaIntegration::createPlayer(QMediaPlayer *player)
{
    return new QFFmpegMediaPlayer(player);
}

QMaybe<QPlatformCamera *> QFFmpegMediaIntegration::createCamera(QCamera *camera)
{
#ifdef Q_OS_DARWIN
    return new QAVFCamera(camera);
#elif defined(Q_OS_ANDROID)
    return new QAndroidCamera(camera);
#elif QT_CONFIG(linux_v4l)
    return new QV4L2Camera(camera);
#elif defined(Q_OS_WINDOWS)
    return new QWindowsCamera(camera);
#else
    Q_UNUSED(camera);
    return nullptr;//new QFFmpegCamera(camera);
#endif
}

QPlatformScreenCapture *QFFmpegMediaIntegration::createScreenCapture(QScreenCapture *screenCapture)
{
#if QT_CONFIG(cpp_winrt)
    if (QFFmpegScreenCaptureUwp::isSupported())
        return new QFFmpegScreenCaptureUwp(screenCapture);
#endif

#if QT_CONFIG(xlib)
    if (QX11ScreenCapture::isSupported())
        return new QX11ScreenCapture(screenCapture);
#endif

#if defined(Q_OS_WINDOWS)
    return new QFFmpegScreenCaptureDxgi(screenCapture);
#elif defined(Q_OS_MACOS) // TODO: probably use it for iOS as well
    return new QAVFScreenCapture(screenCapture);
#else
    return new QFFmpegScreenCapture(screenCapture);
#endif
}

QMaybe<QPlatformMediaRecorder *> QFFmpegMediaIntegration::createRecorder(QMediaRecorder *recorder)
{
    return new QFFmpegMediaRecorder(recorder);
}

QMaybe<QPlatformImageCapture *> QFFmpegMediaIntegration::createImageCapture(QImageCapture *imageCapture)
{
    return new QFFmpegImageCapture(imageCapture);
}

QMaybe<QPlatformVideoSink *> QFFmpegMediaIntegration::createVideoSink(QVideoSink *sink)
{
    return new QFFmpegVideoSink(sink);
}

QMaybe<QPlatformAudioInput *> QFFmpegMediaIntegration::createAudioInput(QAudioInput *input)
{
    return new QFFmpegAudioInput(input);
}

#ifdef Q_OS_ANDROID
Q_DECL_EXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void * /*reserved*/)
{
    static bool initialized = false;
    if (initialized)
        return JNI_VERSION_1_6;
    initialized = true;

    QT_USE_NAMESPACE
    void *environment;
    if (vm->GetEnv(&environment, JNI_VERSION_1_6))
        return JNI_ERR;

    // setting our javavm into ffmpeg.
    if (av_jni_set_java_vm(vm, nullptr))
        return JNI_ERR;

    if (!QAndroidCamera::registerNativeMethods())
        return JNI_ERR;

    return JNI_VERSION_1_6;
}
#endif

QT_END_NAMESPACE

#include "qffmpegmediaintegration.moc"
