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
    QTextStream out;
    if (!bExit)
        out.setDevice(&logFile);

    qDebug() << tr("start test");
    while (!bExit) {
//        model.lock.lock();
//        model.on.wait(&model.lock);

        if (model.bAuto) {
            qDebug() << tr("start auto test");
        }
        else {
            qDebug() << tr("start manual test");
        }

//        model.lock.unlock();

        locker.lock();
        cond.wait(&locker, 1000);
        locker.unlock();
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

                if (obj.contains(QString("TestName"))) {
//                    qDebug() << "TestName: " << obj.value(QString("TestName"));
                    testItem.itemName = obj.value(QString("TestName")).toString();
                }
                if(obj.contains(QString("TestProcedures"))) {
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
