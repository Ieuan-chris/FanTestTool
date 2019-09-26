#ifndef SERIALPORTHELPER_H
#define SERIALPORTHELPER_H

#include <QSerialPort>

class SerialPortHelper
{
    // 为了使用信号
public:
    QSerialPort *serialPort;

public:
    SerialPortHelper(const QString &com) : serialPort(new QSerialPort(com)) {
        serialPort->open(QIODevice::ReadWrite);
        serialPort->setBaudRate(QSerialPort::Baud9600);
        serialPort->setParity(QSerialPort::NoParity);
        serialPort->setStopBits(QSerialPort::OneStop);
        serialPort->setDataBits(QSerialPort::Data8);
    }

    ~SerialPortHelper() {
        serialPort->close();
        delete serialPort;
    }
};

#endif // SERIALPORTHELPER_H
