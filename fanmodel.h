#ifndef FANMODEL_H
#define FANMODEL_H

#include <QWaitCondition>
#include <QMutex>
#include <QString>

struct FanModel
{
//    QMutex lock;
//    QWaitCondition on;
    int fanSpd;
    bool bForward;
    QString logFile;
    QString configFile;
    QString serialPortName;
    bool bAuto;
};

#endif // FANMODEL_H
