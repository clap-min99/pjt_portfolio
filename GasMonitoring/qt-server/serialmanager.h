#ifndef SERIALMANAGER_H
#define SERIALMANAGER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QStringList>

/**
 * @brief STM32 MCU와의 UART 시리얼 통신 및 가스 ADC 데이터 파싱을 담당하는 클래스
 */
class SerialManager : public QObject {
    Q_OBJECT
public:
    explicit SerialManager(QObject* parent = nullptr);
    ~SerialManager();

    QStringList availablePorts() const;
    bool connectPort(const QString& portName, qint32 baudRate = QSerialPort::Baud115200);
    void disconnectPort();
    bool isConnected() const;

    bool sendCommand(const QString& cmd);
    bool sendChar(char cmd);

signals:
    void dataReceived(int adcValue);
    void rawLineReceived(const QString& line);
    void statusMessage(const QString& msg, bool isError);
    void connectionStateChanged(bool isConnected);

private slots:
    void onReadyRead();
    void onErrorOccurred(QSerialPort::SerialPortError error);

private:
    QSerialPort* m_serialPort;
};

#endif // SERIALMANAGER_H