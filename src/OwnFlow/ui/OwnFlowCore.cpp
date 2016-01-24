#include <QDebug>

#include "OwnFlowCore.h"
#include "OwnFlowWidget.h"
#include <QSignalSpy>


hw::OwnFlowCore::OwnFlowCore() {
}

hw::OwnFlowCore::~OwnFlowCore() {
    captureThread.quit();
    captureThread.wait();

//    delete OwnFlowWidget::instance();
}

void hw::OwnFlowCore::initialize() {
    QObject::connect(&capture, &Capture::matReady,
                     OwnFlowWidget::instance(), &OwnFlowWidget::setImage);
    capture.moveToThread(&captureThread);
    captureThread.start();

    QMetaObject::invokeMethod(&capture, "start");
}
