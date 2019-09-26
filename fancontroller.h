#ifndef FANCONTROLLER_H
#define FANCONTROLLER_H

#include <QThread>
#include <fanmodel.h>
#include <QWaitCondition>
#include <QMutex>
#include <QTextStream>
#include <QSerialPort>
#include <QDataStream>


#include <QVector>

#include "testitem.h"

class FanController : public QThread
{
    Q_OBJECT

public:
//    fanController();
    explicit FanController(FanModel &model);

    enum Error {JSON_PARSE_ERROR, LOG_FILE_OPEN_ERROR,PORT_OPEN_ERROR, COMMAND_SEND_ERROR};
    Q_ENUM(Error)
    static unsigned short crc16_table_256(unsigned short sum, unsigned char *p, unsigned int len);
//    void set_model(FanModel *model) {
//        this->model = model;
//    }
    static const unsigned short crc16_table[256];

Q_SIGNALS:
    void errorOccured(Error err);
    void requestSerialoutToCurThread(QThread *);
    void updateUserDisp(const QString &text);


private:
    FanModel &model;
    bool bExit;
    bool parseConfig(QVector<TestItem> &testItems);
    bool sendSubproInstruction(const ItemProcedure &subItem, bool isStopMachine = false);
    QWaitCondition cond;
    QMutex locker;
    QTextStream out;
    QSerialPort serialout;

    // QThread interface
protected:
    void run() override;

public Q_SLOTS:
    void wakeAndWaitForEnd(void);
    void receiveData(void);
    void processSentData(qint64 bytes);
    void moveSerialoutToOtherThread(QThread * target);


};



#endif // FANCONTROLLER_H
