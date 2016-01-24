/**
 * @file
 *   @brief Definition of class OwnFlowWidget
 *   @author Helmut Wolf <helmut.wolf@student.uibk.ac.at>
 *
 */

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp> 

#include "ui_OwnFlowWidget.h"
#include "OwnFlowWidget.h"
#include "UASManager.h"

OwnFlowWidget::OwnFlowWidget(QWidget *parent)
    : QWidget(parent)
    , curr(-1)
    , ui(new Ui::OwnFlowWidget)
{
    ui->setupUi(this);

    // Get current MAV list
    QList<UASInterface*> systems = UASManager::instance()->getUASList();

    // Add each of them
    foreach (UASInterface* sys, systems) {
        addUAS(sys);
    }

    // Setup MAV connections
    connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)), this, SLOT(addUAS(UASInterface*)));
    connect(UASManager::instance(), SIGNAL(activeUASSetListIndex(int)), this, SLOT(selectUAS(int)));
//    this->setVisible(true);
}

OwnFlowWidget* OwnFlowWidget::instance(QWidget *parent) {
    static OwnFlowWidget* _instance = 0;
    if(_instance == 0)
    {
        _instance = new OwnFlowWidget(parent);
    }
    return _instance;
}


OwnFlowWidget::~OwnFlowWidget()
{
    delete ui;
}

void OwnFlowWidget::paintEvent(QPaintEvent *ev) {
    QPainter p(this);
    p.drawImage(0, 0, m_img);
    m_img = QImage();
}


/**
 *
 * @param uas System to add to list
 */
void OwnFlowWidget::addUAS(UASInterface* uas)
{
    // QGCParamWidget* param = new QGCParamWidget(uas, this);
    // paramWidgets->insert(uas->getUASID(), param);
    // m_ui->stackedWidget->addWidget(param);


    // QGCSensorSettingsWidget* sensor = new QGCSensorSettingsWidget(uas, this);
    // m_ui->sensorSettings->addWidget(sensor);

    // // Set widgets as default
    // if (curr == -1) {
    //     // Clear
    //     m_ui->sensorSettings->setCurrentWidget(sensor);
    //     m_ui->stackedWidget->setCurrentWidget(param);
    //     curr = 0;
    // }
}

void OwnFlowWidget::selectUAS(int index)
{
    // m_ui->stackedWidget->setCurrentIndex(index);
    // m_ui->sensorSettings->setCurrentIndex(index);
    curr = index;
}

void OwnFlowWidget::setImage(const cv::Mat& img) {
    cv::Mat tmp;
    cv::resize(img, tmp, cv::Size(), 0.3, 0.3, cv::INTER_AREA);
    cv::cvtColor(tmp, tmp, CV_BGR2RGB);
    const QImage image(tmp.data, tmp.cols, tmp.rows, tmp.step,
                       QImage::Format_RGB888, &matDeleter, new cv::Mat(tmp));

    if (!m_img.isNull())
        qDebug() << "UI: OwnFlow dropped frame";
    m_img = image;
    if (m_img.size() != size())
        setFixedSize(m_img.size());
    update();
}
