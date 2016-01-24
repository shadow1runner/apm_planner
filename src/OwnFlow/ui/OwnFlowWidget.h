/**
 * @file
 *   @brief Definition of class OwnFlowWidget
 *   @author Helmut Wolf <helmut.wolf@student.uibk.ac.at>
 *
 */

#ifndef OWNFLOW_OWNFLOWWIDGET_H
#define OWNFLOW_OWNFLOWWIDGET_H

#include <QtWidgets/QWidget>
#include <QImage>
#include <QPainter>
#include <QPaintEvent>
#include <opencv2/core/core.hpp>

#include "UASInterface.h"
#include "QGCParamWidget.h"

namespace Ui {
    class OwnFlowWidget;
}

/**
 * @brief Container class for onboard parameter widgets
 *
 * @see QGCParamWidget
 */
class OwnFlowWidget : public QWidget
{
    Q_OBJECT
    OwnFlowWidget(QWidget *parent = 0);
    Ui::OwnFlowWidget *ui;
    int curr;
    QImage m_img;
    void paintEvent(QPaintEvent *ev);
    static void matDeleter(void* mat) {
        delete static_cast<cv::Mat*>(mat); 
    }
        
public:
    static OwnFlowWidget* instance(QWidget *parent = 0);
    virtual ~OwnFlowWidget();

public slots:
    void addUAS(UASInterface* uas);
    void selectUAS(int index);
    void setImage(const cv::Mat& img);
};

#endif // OWNFLOW_OWNFLOWWIDGET_H
