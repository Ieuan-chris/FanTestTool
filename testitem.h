#ifndef TESTITEM_H
#define TESTITEM_H

#include <QString>
#include <QMap>
#include <QVector>


struct ItemProcedure {
    short spd;
    bool isForward;
    int duration;
};

struct TestItem {
    QString itemName;
    int repeats;
    QMap<QString, ItemProcedure> procedures;
};

#endif // TESTITEM_H
