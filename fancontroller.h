#ifndef FANCONTROLLER_H
#define FANCONTROLLER_H

#include <QThread>
#include <fanmodel.h>
#include <QWaitCondition>
#include <QMutex>
#include <QTextStream>


#include <QVector>

#include "testitem.h"

class FanController : public QThread
{
    Q_OBJECT

public:
//    fanController();
    explicit FanController(FanModel &model);

//    void set_model(FanModel *model) {
//        this->model = model;
//    }

Q_SIGNALS:
    void parseError(void);


private:
    FanModel &model;
    bool bExit;
    bool parseConfig(QVector<TestItem> &testItems);
    bool sendSubproInstruction(const ItemProcedure &subItem);
    QWaitCondition cond;
    QMutex locker;
    QTextStream out;

    // QThread interface
protected:
    void run() override;

public Q_SLOTS:
    void wakeAndWaitForEnd(void);

};

#endif // FANCONTROLLER_H
