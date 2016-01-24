#include "capture.h"
#include <qdebug.h>

hw::Capture::Capture(QObject * parent)
    : QObject(parent)
{}

void hw::Capture::timerEvent(QTimerEvent * ev)  {
    if (ev->timerId()!=m_timer.timerId())
        return;

    cv::Mat frame;
    if (!m_videoCapture->read(frame)) { // blocks until a new frame is ready
        m_timer.stop();
        return;
    }
    emit matReady(frame);
}

void hw::Capture::triggerMatReady() {
    cv::Mat frame(2000,1300, CV_8UC3, cv::Scalar(0,0,255));
    emit matReady(frame);
}

void hw::Capture::start(int cam) {
    if (!m_videoCapture)
        m_videoCapture.reset(new cv::VideoCapture(cam));
    if (m_videoCapture->isOpened()) {
        m_timer.start(20, this);
        emit started();
    }
}

void hw::Capture::stop() {
    m_timer.stop();
}

