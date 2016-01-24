#ifndef OPTICALFLOW_OWNFLOWCORE_H
#define OPTICALFLOW_OWNFLOWCORE_H

#include <QThread>
#include "Capture.h"

namespace hw {
    class OwnFlowCore
    {
        Capture capture;
        QThread captureThread;

    public:
        OwnFlowCore();
        ~OwnFlowCore();
        
        void initialize();
    };
}
#endif // OPTICALFLOW_OWNFLOWCORE_H
