#include "dialog.h"
#include <QApplication>

#include "fanmodel.h"
#include "testitem.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    FanModel model;
    Dialog w(model);
    w.show();

    return a.exec();
}
