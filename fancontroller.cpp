#include <QMessageBox>
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonObject>

#include "fancontroller.h"


const unsigned short FanController::crc16_table[256] =
{
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7, 0x8108,
    0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef, 0x1231, 0x0210,
    0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6, 0x9339, 0x8318, 0xb37b,
    0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de, 0x2462, 0x3443, 0x0420, 0x1401,
    0x64e6, 0x74c7, 0x44a4, 0x5485, 0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee,
    0xf5cf, 0xc5ac, 0xd58d, 0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6,
    0x5695, 0x46b4, 0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d,
    0xc7bc, 0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b, 0x5af5,
    0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12, 0xdbfd, 0xcbdc,
    0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a, 0x6ca6, 0x7c87, 0x4ce4,
    0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41, 0xedae, 0xfd8f, 0xcdec, 0xddcd,
    0xad2a, 0xbd0b, 0x8d68, 0x9d49, 0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13,
    0x2e32, 0x1e51, 0x0e70, 0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a,
    0x9f59, 0x8f78, 0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e,
    0xe16f, 0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e, 0x02b1,
    0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256, 0xb5ea, 0xa5cb,
    0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d, 0x34e2, 0x24c3, 0x14a0,
    0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 0xa7db, 0xb7fa, 0x8799, 0x97b8,
    0xe75f, 0xf77e, 0xc71d, 0xd73c, 0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657,
    0x7676, 0x4615, 0x5634, 0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9,
    0xb98a, 0xa9ab, 0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882,
    0x28a3, 0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92, 0xfd2e,
    0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9, 0x7c26, 0x6c07,
    0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1, 0xef1f, 0xff3e, 0xcf5d,
    0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8, 0x6e17, 0x7e36, 0x4e55, 0x5e74,
    0x2e93, 0x3eb2, 0x0ed1, 0x1ef0,
};


FanController::FanController(FanModel &model) : model(model), bExit(false), cond(), locker()
{
    connect(&serialout, &QSerialPort::bytesWritten, this, &FanController::processSentData);
//    connect(&serialout, &QSerialPort::readyRead, this, &FanController::receiveData);
    connect(this, &FanController::requestSerialoutToCurThread, this, &FanController::moveSerialoutToOtherThread);
}


void FanController::run()
{
    bExit = false;
    QVector<TestItem> testItems;
    if(model.bAuto && !parseConfig(testItems)) {
        emit errorOccured(JSON_PARSE_ERROR);

        locker.lock();
        cond.wait(&locker);
        locker.unlock();
    }

    foreach(auto testItem, testItems) {
        qDebug() << testItem.itemName;
        foreach(auto key, testItem.procedures.keys()) {
              qDebug() << key << ": " << testItem.procedures.value(key).spd;
        }
    }


    QFile logFile(model.logFile);

    if (!bExit && !logFile.open(QFile::WriteOnly | QFile::Append)) {
        emit errorOccured(LOG_FILE_OPEN_ERROR);
        locker.lock();
        cond.wait(&locker);
        locker.unlock();
    }
    if (!bExit) {
        out.setDevice(&logFile);
        serialout.setPortName(model.serialPortName);

        if (!serialout.open(QIODevice::ReadWrite)) {
            qDebug() << serialout.error();
            emit errorOccured(PORT_OPEN_ERROR);

            locker.lock();
            cond.wait(&locker);
            locker.unlock();
        }

        serialout.setBaudRate(QSerialPort::Baud9600);
        serialout.setStopBits(QSerialPort::OneStop);
        serialout.setDataBits(QSerialPort::Data8);
        serialout.setParity(QSerialPort::NoParity);

        qDebug() << "before: " << serialout.thread();

        if (serialout.thread() != currentThread())
            emit requestSerialoutToCurThread(currentThread());
        while(serialout.thread() != currentThread()) {
            yieldCurrentThread();
        }

    }


    while (!bExit) {

        if (model.bAuto) {
            qDebug() << "start auto test";
            foreach(auto testItem, testItems) {
                out << "TestItem: " << testItem.itemName << ":" << endl;
                for (int i = 0; i < testItem.repeats; ++i) {
                    foreach(auto key, testItem.procedures.keys()) {
                        out << '\t' << "[ " << i << " ] SubItem: " << key << ":" << endl;
                        if(!sendSubproInstruction(testItem.procedures.value(key))) {
                            emit errorOccured(COMMAND_SEND_ERROR);
                        }

                        locker.lock();
                        cond.wait(&locker, static_cast<unsigned long>(testItem.procedures.value(key).duration));
                        locker.unlock();

                        if (bExit) {
                            if(!sendSubproInstruction(testItem.procedures.value(key), true)) {
                                emit errorOccured(COMMAND_SEND_ERROR);
                            }
                            break;
                        }
                    }
                    if (bExit)
                        break;
                }
                if (bExit)
                    break;
            }
        }
        else {
            ItemProcedure single;
            single.duration = 0;
            single.spd = static_cast<short>(model.fanSpd);
            single.isForward = model.bForward;
            if(!sendSubproInstruction(single)) {
                emit errorOccured(COMMAND_SEND_ERROR);
            }

            locker.lock();
            cond.wait(&locker);
            locker.unlock();

            if(!sendSubproInstruction(single, true)) {
                emit errorOccured(COMMAND_SEND_ERROR);
            }
        }

        out.flush();


    }

    qDebug() << tr("release controller resource");
    // 释放资源
    if (logFile.isOpen()) {
        logFile.close();
    }
    if (serialout.isOpen())
        serialout.close();
}

bool FanController::parseConfig(QVector<TestItem> &testItems)
{
    bool ret = false;
    // Todo: parse
    QFile loadFile(model.configFile);

    if(!loadFile.open(QIODevice::ReadOnly))
    {
        qDebug() << "could't open projects json";
        return ret;
    }

    QByteArray allData = loadFile.readAll();
    loadFile.close();

    QJsonParseError json_error;
    QJsonDocument jsonDoc(QJsonDocument::fromJson(allData, &json_error));

    if(json_error.error != QJsonParseError::NoError)
    {
        qDebug() << "json error: " << json_error.error;
        return ret;
    }

    if (jsonDoc.isArray()) {
        QJsonArray tests = jsonDoc.array();

        foreach(auto test, tests) {
            TestItem testItem;
            ItemProcedure itemProcedure;
            if (test.isObject()) {
                QJsonObject obj = test.toObject();

                if (!obj.contains(QString("TestName")) || !obj.contains(QString("Repeats")) || !obj.contains(QString("TestProcedures"))) {
                    ret = false;
                    return ret;
                }
                testItem.itemName = obj.value(QString("TestName")).toString();
                testItem.repeats = obj.value(QString("Repeats")).toInt();

                QJsonArray procedures = obj.value(QString("TestProcedures")).toArray();
                QStringList pduList;
                pduList << "speed" << "forward" << "duration";
                foreach(auto procedure, procedures) {
                    if (procedure.isObject()) {
                        QJsonObject proObj = procedure.toObject();
                        QString subProcedure = proObj.keys().at(0);
                        QJsonObject subPro = proObj.value(subProcedure).toObject();
                        foreach(auto sPdu, pduList) {
                            if (subPro.contains(sPdu)) {
                                if (sPdu == "forward") {
                                    itemProcedure.isForward = subPro.value(sPdu).toBool();
                                } else if (sPdu == "speed"){
                                    itemProcedure.spd = static_cast<short>(subPro.value(sPdu).toInt());
                                } else {
                                    itemProcedure.duration = subPro.value(sPdu).toInt();
                                }
                            }
                        }
                        testItem.procedures.insert(subProcedure, itemProcedure);
                    }
                }
                testItems.append(testItem);
                ret = true;

            }
        }
    }

    return ret;
}

void FanController::wakeAndWaitForEnd()
{
    bExit = true;
    locker.lock();
    cond.wakeOne();
    locker.unlock();
}

bool FanController::sendSubproInstruction(const ItemProcedure &subItem, bool isStopMachine)
{
    bool ret = true;
    if (out.device())
        out << '\t' << '\t' << "send cmd: " << "speed=" << subItem.spd << ", forward="
            << subItem.isForward << ", duration=" << subItem.duration << endl;

    emit updateUserDisp(QString("[CMD] motor: %1, speed: %2, forward: %3").arg(!isStopMachine).arg(subItem.spd).arg(subItem.isForward));

    // package the data frame
    QString spdStr = QString::number(subItem.spd, 16);
    QString dirStr = QString::number(subItem.isForward, 16);
    QString cmdStr = QString::number(0, 16);
    QString cmdParamStr = QString::number(1, 16);
    QString lightSettingStr = QString::number(0, 16);
    QString lightIntensityStr = QString::number(0, 16);
    QString motorSettingStr = isStopMachine ? QString::number(0, 16) : QString::number(1, 16);

    QString pack = cmdStr.rightJustified(2, '0') + cmdParamStr.rightJustified(2, '0') +
            lightSettingStr.rightJustified(2, '0') + lightIntensityStr.rightJustified(2, '0') +
            motorSettingStr.rightJustified(2, '0') + spdStr.rightJustified(2, '0') +
            dirStr.rightJustified(2, '0') + QString::number(0, 16).rightJustified(44, '0');

    pack = QString::number(pack.size()/2+4, 16).rightJustified(4, '0') + pack;


    QString temp(pack);
    unsigned char datas[33];
    bool ok;
    for (int i = 60; i > 0; i-=2) {
        QString s = temp.chopped(i);
        temp = temp.right(i);
        datas[(62-i)/2-1] = static_cast<unsigned char>(s.toInt(&ok, 16));
    }
    unsigned short checkSum = crc16_table_256(0xFFFF, datas, 31);
    QString checkSumStr = QString::number(checkSum, 16);

    pack = pack + checkSumStr.rightJustified(4, '0');


    char*  ch;
    QByteArray ba = pack.toLatin1(); // must
    ch=ba.data();

    QByteArray sDatas = QByteArray::fromHex(ch);

    qDebug() << "sDatas: " << sDatas;

//    serialout.setReadBufferSize(100);

//    emit requestSerialDataWrite(ch, pack.size());
    qDebug() << ba;
    if ( -1 == serialout.write(sDatas.data(), 33))
        ret = false;

    if (!serialout.waitForBytesWritten(500))
        ret = false;


    QByteArray rcvData;
    while (serialout.waitForReadyRead(1000)) {
        rcvData += serialout.readAll().toHex();
        if (rcvData.length() >=4 && rcvData.length() >= rcvData.mid(0, 4).toUShort(&ok, 16) * 2) {
            ret = true;
            break;
        }
    }




    qDebug() << "rcvData: " << rcvData;

//    qDebug() << "end :";
    return ret;
}

void FanController::receiveData(void)
{
    qDebug() << "start receive data: ";

    QByteArray rcvData = serialout.read(100);
    qDebug() << "rcvData: " << rcvData;
}

void FanController::processSentData(qint64 bytes)
{
    qDebug() << "had sent data : "<< bytes << " bytes";
}



unsigned short FanController::crc16_table_256(unsigned short sum, unsigned char *p, unsigned int len)
{
    while (len--)
    {
        sum = static_cast<unsigned short>(crc16_table[(sum >> 8) ^ *p++] ^ (sum << 8));
    }

    return sum;
}

void FanController::moveSerialoutToOtherThread(QThread * target)
{
    serialout.moveToThread(target);
}
