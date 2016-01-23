/**
 * @file
 *   @brief Definition of class OwnFlowInterface
 *   @author Helmut Wolf <helmut.wolf@student.uibk.ac.at>
 *
 */

#ifndef OWNFLOW_OWNFLOWINTERFACE_H
#define OWNFLOW_OWNFLOWINTERFACE_H

#include <QtWidgets/QWidget>

#include "UASInterface.h"
#include "QGCParamWidget.h"

namespace Ui {
    class OwnFlowInterface;
}

/**
 * @brief Container class for onboard parameter widgets
 *
 * @see QGCParamWidget
 */
class OwnFlowInterface : public QWidget
{
    Q_OBJECT
public:
    explicit OwnFlowInterface(QWidget *parent = 0);
    virtual ~OwnFlowInterface();

public slots:
    void addUAS(UASInterface* uas);
    void selectUAS(int index);

protected:
    int curr;
private:
    Ui::OwnFlowInterface *ui;
};

#endif // OWNFLOW_OWNFLOWINTERFACE_H
