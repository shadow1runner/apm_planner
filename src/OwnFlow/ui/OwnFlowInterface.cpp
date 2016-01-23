/**
 * @file
 *   @brief Definition of class OwnFlowInterface
 *   @author Helmut Wolf <helmut.wolf@student.uibk.ac.at>
 *
 */

#include <QTreeWidget>

#include "ui_OwnFlowInterface.h"
#include "OwnFlowInterface.h"
#include "UASManager.h"

OwnFlowInterface::OwnFlowInterface(QWidget *parent)
    : QWidget(parent)
    , curr(-1)
    , ui(new Ui::OwnFlowInterface)
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
    this->setVisible(true);
}

OwnFlowInterface::~OwnFlowInterface()
{
    delete ui;
}

void OwnFlowInterface::selectUAS(int index)
{
    // m_ui->stackedWidget->setCurrentIndex(index);
    // m_ui->sensorSettings->setCurrentIndex(index);
    curr = index;
}

/**
 *
 * @param uas System to add to list
 */
void OwnFlowInterface::addUAS(UASInterface* uas)
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
