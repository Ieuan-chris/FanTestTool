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

    qDebug() << "Is it json array : " << jsonDoc.isArray();

    if (jsonDoc.isArray()) {
        QJsonArray tests = jsonDoc.array();

        foreach(auto test, tests) {
            if (test.isObject()) {
                QJsonObject obj = test.toObject();

                if (obj.contains(QString("TestName")))
                    qDebug() << "TestName: " << obj.value(QString("TestName"));
                if(obj.contains(QString("TestProcedures")))
                    qDebug() << "TestProcedures: " << obj.value(QString("TestProcedures"));
            }
        }
    } else if (jsonDoc.isObject()) {
        qDebug() << "jsonDoc is object: " << true;
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
