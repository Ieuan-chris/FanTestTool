#include <QStringList>
#include <QDebug>
#include <QMessageBox>
#include <QThread>
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>
#include <QtSerialPort/QSerialPortInfo>

#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(FanModel &model, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::dialog),
    model(model)
{
    ui->setupUi(this);
    QStringList speeds;
    speeds << "Level 1" << "Level 2" << "Level 3" << "Level 4" << "Level 5" << "Level 6";
    ui->speedLevelCombox->addItems(speeds);
    ui->speedLevelCombox->setCurrentIndex(0);
    ctr = new FanController(model);

    ui->serialPortsSelector->installEventFilter(this);

    connect(ui->startup, SIGNAL(stateChanged(int)), this, SLOT(toggleFanStatu(int)));
    connect(ctr, &FanController::errorOccured, this, &Dialog::processError);
    connect(this, SIGNAL(terminateTesting()), ctr, SLOT(wakeAndWaitForEnd()));
    connect(ui->RecordSelectorBtn, SIGNAL(clicked()), this, SLOT(setRecordFile()));
    connect(ui->configSettingBtn, SIGNAL(clicked()), this, SLOT(setConfigFile()));
}

Dialog::~Dialog()
{
    delete ui;
}


void Dialog::toggleFanStatu (int state)
{
    if (state == Qt::Checked) {
        // 判断输入是否正确
        if (ui->RecordFilePathText->text().isEmpty() || (ui->autoModeEnableBox->isChecked() && ui->configFilePathText->text().isEmpty())) {
            QMessageBox msgBox;
            msgBox.setText(tr("请选择配置文件和记录文件."));
            msgBox.exec();

            ui->startup->setCheckState(Qt::Unchecked);
            return;
        }

        qDebug() << "current port number: " << ui->serialPortsSelector->currentIndex() << "current port name: " << ui->serialPortsSelector->currentText();
        if (ui->serialPortsSelector->currentIndex() == -1) {
            QMessageBox msgBox;
            msgBox.setText(tr("请选择串口."));
            msgBox.exec();

            ui->startup->setCheckState(Qt::Unchecked);
            return;
        }

        model.bAuto = ui->autoModeEnableBox->isChecked();
        model.fanSpd = ui->speedLevelCombox->currentIndex() + 1;
        model.logFile = ui->RecordFilePathText->text();
        model.bForward = ui->directionBox->isChecked();
        model.configFile = ui->configFilePathText->text();
        model.serialPortName = ui->serialPortsSelector->currentText();

        disableWidgets();
        ctr->start();

    }
    else if (state == Qt::Unchecked){
        qDebug() << tr("delete thread");
        emit terminateTesting();
        QThread::currentThread()->yieldCurrentThread();
//        ctr->exit(1);
        ctr->wait();
        enableWidgets();
    }
//    if(this->ctr->isRunning()) {
//        qDebug() << "delete controller";
//        this->ctr->deleteLater();
//        qDebug() << "enable widgets";
//        enableWidgets();
//    }
//    else {
//        // 尝试获取锁
////        if(model.lock.try_lock()) {
//        disableWidgets();
////            model.on.wakeOne();
////        }

//        this->ctr->start();
//    }

}

void Dialog::enableWidgets(void)
{
    qDebug() << "Enable Widgets";
//    ui->verticalLayout->setEnabled(true);
    ui->directionBox->setEnabled(true);
    ui->configSettingBtn->setEnabled(true);
    ui->speedLevelCombox->setEnabled(true);
    ui->RecordSelectorBtn->setEnabled(true);
    ui->autoModeEnableBox->setEnabled(true);
    ui->serialPortsSelector->setEnabled(true);
//    ui->startup->setEnabled(true);
}

void Dialog::disableWidgets(void)
{
    qDebug() << "Disable Widgets";
    ui->directionBox->setEnabled(false);
    ui->configSettingBtn->setEnabled(false);
    ui->speedLevelCombox->setEnabled(false);
    ui->RecordSelectorBtn->setEnabled(false);
    ui->autoModeEnableBox->setEnabled(false);
    ui->serialPortsSelector->setEnabled(false);
}

void Dialog::enableAutoMode(bool)
{
    qDebug() << "Enable AutoMode";
}

void Dialog::setFanSpeed(int)
{
    qDebug() << "Set Fan Speed";
}

void Dialog::setFanDirection(bool)
{
    qDebug() << "Set Fan Direction";
}

void Dialog::setRecordFile()
{
    qDebug() << "Set Record File";

    QFileDialog fileDialog(this);
    //定义文件对话框标题
    fileDialog.setWindowTitle(tr("打开图片"));
    //设置默认文件路径
    fileDialog.setDirectory(".");
    //设置文件过滤器
    fileDialog.setNameFilter(tr("Images(*.txt)"));
    //设置可以选择多个文件,默认为只能选择一个文件QFileDialog::ExistingFiles
    fileDialog.setFileMode(QFileDialog::ExistingFile);
    //设置视图模式
    fileDialog.setViewMode(QFileDialog::Detail);
    //设置初始文件夹位置
    fileDialog.setDirectory(QDir::currentPath());
    //打印所有选择的文件的路径
    if(fileDialog.exec())
    {
        model.logFile = fileDialog.selectedFiles().at(0);
    }
    qDebug() << model.logFile;

    ui->RecordFilePathText->setText(model.logFile);
}

void Dialog::setConfigFile()
{
    qDebug() << "Set Config File";

    QFileDialog fileDialog(this);
    //定义文件对话框标题
    fileDialog.setWindowTitle(tr("打开图片"));
    //设置默认文件路径
    fileDialog.setDirectory(".");
    //设置文件过滤器
    fileDialog.setNameFilter(tr("Images(*.txt)"));
    //设置可以选择多个文件,默认为只能选择一个文件QFileDialog::ExistingFiles
    fileDialog.setFileMode(QFileDialog::ExistingFile);
    //设置视图模式
    fileDialog.setViewMode(QFileDialog::Detail);
    //设置初始文件夹位置
    fileDialog.setDirectory(QDir::currentPath());
    //打印所有选择的文件的路径
    if(fileDialog.exec())
    {
        model.configFile = fileDialog.selectedFiles().at(0);
    }
    qDebug() << model.configFile;

    ui->configFilePathText->setText(model.configFile);

}

void Dialog::processError(ctrErr err)
{

    QMessageBox msgBox;
//    ctr->terminate();
    if (err == FanController::JSON_PARSE_ERROR)
        msgBox.setText(tr("配置文件解析错误！"));
    else if (err == FanController::LOG_FILE_OPEN_ERROR) {
        msgBox.setText(tr("不能打开日志文件！"));
    }
    else if (err == FanController::PORT_OPEN_ERROR) {
        msgBox.setText(tr("不能打开串口！"));
    }
    else if (err == FanController::COMMAND_SEND_ERROR) {
        msgBox.setText(tr("串口通信失败！"));
    }
    else {
        msgBox.setText(tr("其他错误！！！"));
    }
    msgBox.exec();

    emit terminateTesting();
    QThread::currentThread()->yieldCurrentThread();
    ctr->wait();
    enableWidgets();
}

bool Dialog::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->serialPortsSelector)
    {
        if (event->type() == QEvent::FocusIn)
        {
            qDebug() << "focus in serialportsslector";
            foreach(auto port, QSerialPortInfo::availablePorts()) {
                if (ui->serialPortsSelector->findText(port.portName()) == -1)
                    ui->serialPortsSelector->addItem(port.portName());
            }
            if (ui->serialPortsSelector->count() != QSerialPortInfo::availablePorts().count()) {
                ui->serialPortsSelector->clear();
                foreach(auto port, QSerialPortInfo::availablePorts()) {
                    if (ui->serialPortsSelector->findText(port.portName()) == -1)
                        ui->serialPortsSelector->addItem(port.portName());
                }
            }
        }
        if (event->type() == QEvent::FocusOut) {
            qDebug() << "focus out serialportsslector";
        }
    }

    return QDialog::eventFilter(watched,event);
}

