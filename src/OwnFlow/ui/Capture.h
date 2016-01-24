#ifndef OPTICALFLOW_CAPTURE_H
#define OPTICALFLOW_CAPTURE_H

#include <QObject>
#include <QBasicTimer>
#include <QTimerEvent>
#include <opencv2/videoio.hpp>

namespace hw {
    class Capture : public QObject {
        Q_OBJECT
        QBasicTimer m_timer;
        QScopedPointer<cv::VideoCapture> m_videoCapture;

        void timerEvent(QTimerEvent * ev);

    public:
        Capture(QObject * parent = 0);
        void triggerMatReady();

    public slots:
        void start(int cam = 0);
        void stop();

    signals:
        void started();
        void matReady(const cv::Mat &);
    };  
}

#endif // OPTICALFLOW_CAPTURE_H
