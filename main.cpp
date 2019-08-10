#include "dialog.h"
#include <QApplication>
#include <QMetaType>

#include "fanmodel.h"
#include "testitem.h"

int main(int argc, char *argv[])
{
    qRegisterMetaType<FanController::Error>("Error");
    QApplication a(argc, argv);
    FanModel model;
    Dialog w(model);
    w.show();

    return a.exec();
}
