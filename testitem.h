#ifndef TESTITEM_H
#define TESTITEM_H

#include <QString>
#include <QMap>
#include <QVector>


struct ItemProcedure {
    ushort spd;
    bool isForward;
    uint time_ms;
};

struct TestItem {
    QString itemName;
    QVector<QMap<QString, ItemProcedure> > procedures;
};

#endif // TESTITEM_H
