#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QDir>

#include "fanmodel.h"
#include "fancontroller.h"


namespace Ui {
class dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(FanModel &model, QWidget *parent = nullptr);
    ~Dialog();

private:
    Ui::dialog *ui;
    FanModel &model;
    FanController *ctr;
    void enableWidgets(void);
    void disableWidgets(void);

Q_SIGNALS:
    void testEnable(void);

public Q_SLOTS:
    void toggleFanStatu (int);
    void enableAutoMode(bool);
    void setFanSpeed(int);
    void setFanDirection(bool);
    void setRecordFile();
    void setConfigFile();
    void configFileParseError(void);

};

#endif // DIALOG_H
