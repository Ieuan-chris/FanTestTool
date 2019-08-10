#include <QMessageBox>
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonObject>

#include "fancontroller.h"


FanController::FanController(FanModel &model) : model(model), bExit(false), cond(), locker()
{

}


void FanController::run()
{
    bExit = false;
    QVector<TestItem> testItems;
    if(model.bAuto && !parseConfig(testItems)) {
        // 弹出错误消息框，删除线程
        bExit = true;
        emit parseError();
    }

    foreach(auto testItem, testItems) {
        qDebug() << testItem.itemName;
        foreach(auto key, testItem.procedures.keys()) {
              qDebug() << key << ": " << testItem.procedures.value(key).spd;
        }
    }

    QFile logFile(model.logFile);

    if (!bExit && !logFile.open(QFile::WriteOnly | QFile::Append)) {
//        QTextStream out(&logFile);
//        out << "Result: " << qSetFieldWidth(10) << left << 3.14 << 2.7;
        // writes "Result: 3.14      2.7       "
        bExit = true;
        emit parseError();
    }
    if (!bExit)
        out.setDevice(&logFile);

    qDebug() << tr("start test");
    while (!bExit) {
//        model.lock.lock();
//        model.on.wait(&model.lock);

        if (model.bAuto) {
            qDebug() << "start auto test";
            foreach(auto testItem, testItems) {
                out << "TestItem: " << testItem.itemName << ":" << endl;
                for (int i = 0; i < testItem.repeats; ++i) {
                    foreach(auto key, testItem.procedures.keys()) {
                        out << '\t' << "[ " << i << " ] SubItem: " << key << ":" << endl;
                        if(!sendSubproInstruction(testItem.procedures.value(key))) {
                            qDebug() << "send cmd failed";
                            bExit = true;
                            emit parseError();
                        }

                        locker.lock();
                        cond.wait(&locker, static_cast<unsigned long>(testItem.procedures.value(key).duration));
                        locker.unlock();

                        if (bExit)
                            break;
                    }
                    if (bExit)
                        break;
                }
                if (bExit)
                    break;
            }
        }
        else {
            qDebug() << tr("start manual test");
            ItemProcedure single;
            single.duration = 0;
            single.spd = static_cast<short>(model.fanSpd);
            single.isForward = model.bForward;
            if(!sendSubproInstruction(single)) {
                qDebug() << "send cmd failed";
                bExit = true;
                emit parseError();
            }

            locker.lock();
            cond.wait(&locker);
            locker.unlock();
        }

        out.flush();

//        model.lock.unlock();

    }

    qDebug() << tr("release controller resource");
    // 释放资源
    if (logFile.isOpen()) {
        logFile.close();
    }
}

bool FanController::parseConfig(QVector<TestItem> &testItems)
{
    bool ret = false;
    // Todo: parse
    qDebug() << model.configFile;
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
//                    qDebug() << "TestName: " << obj.value(QString("TestName"));
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
//                            qDebug() << "Procedu :" << proObj.keys();
                        QString subProcedure = proObj.keys().at(0);
                        QJsonObject subPro = proObj.value(subProcedure).toObject();
                        foreach(auto sPdu, pduList) {
                            if (subPro.contains(sPdu)) {
//                                    qDebug() << subPro.value(sPdu);
                                if (sPdu == "forward") {
                                    itemProcedure.isForward = subPro.value(sPdu).toBool();
                                } else if (sPdu == "speed"){
                                    itemProcedure.spd = static_cast<short>(subPro.value(sPdu).toInt());
                                } else {
                                    itemProcedure.duration = subPro.value(sPdu).toInt();
                                }
                            }
                        }
                        qDebug() << subProcedure <<itemProcedure.spd << itemProcedure.isForward << itemProcedure.duration;
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

bool FanController::sendSubproInstruction(const ItemProcedure &subItem)
{
    if (out.device())
        out << '\t' << '\t' << "send cmd: " << "speed=" << subItem.spd << ", forward="
            << subItem.isForward << ", duration=" << subItem.duration << endl;

    qDebug() << "Send cmd: " << "speed=" << subItem.spd << ", forward="
             << subItem.isForward << ", duration=" << subItem.duration << endl;

    return true;
}
