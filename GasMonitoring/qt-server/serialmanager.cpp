#include "serialmanager.h"
#include "logger.h"

SerialManager::SerialManager(QObject* parent)
    : QObject(parent)
    , m_serialPort(new QSerialPort(this))
{
    connect(m_serialPort, &QSerialPort::readyRead, this, &SerialManager::onReadyRead);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &SerialManager::onErrorOccurred);
}

SerialManager::~SerialManager()
{
    disconnectPort();
}

QStringList SerialManager::availablePorts() const
{
    QStringList ports;
    const auto portInfos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& info : portInfos) {
        ports.append(info.portName());
    }
    return ports;
}

bool SerialManager::connectPort(const QString& portName, qint32 baudRate)
{
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
    }

    m_serialPort->setPortName(portName);
    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (m_serialPort->open(QIODevice::ReadWrite)) {
        m_serialPort->clear();
        emit statusMessage(Logger::format(LogCategory::Serial, LogLevel::Info,
                               QString("시리얼 포트 연결 성공: %1 (%2 bps)").arg(portName).arg(baudRate)),
            false);
        emit connectionStateChanged(true);
        return true;
    }

    emit statusMessage(Logger::format(LogCategory::Serial, LogLevel::Error,
                           QString("시리얼 포트 연결 실패: %1 (%2)").arg(portName).arg(m_serialPort->errorString())),
        true);
    emit connectionStateChanged(false);
    return false;
}

void SerialManager::disconnectPort()
{
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
        emit statusMessage(Logger::format(LogCategory::Serial, LogLevel::Info, "시리얼 포트 연결 해제됨"), false);
        emit connectionStateChanged(false);
    }
}

bool SerialManager::isConnected() const
{
    return m_serialPort->isOpen();
}

bool SerialManager::sendCommand(const QString& cmd)
{
    if (!m_serialPort->isOpen())
        return false;

    return m_serialPort->write(cmd.toUtf8()) != -1;
}

bool SerialManager::sendChar(char cmd)
{
    if (!m_serialPort->isOpen())
        return false;

    return m_serialPort->write(&cmd, 1) != -1;
}

// 개행 문자 단위 데이터 파싱 및 유효 범위(0~4095) 검증
void SerialManager::onReadyRead()
{
    while (m_serialPort->canReadLine()) {
        QByteArray lineBytes = m_serialPort->readLine();
        QString line = QString::fromUtf8(lineBytes).trimmed();

        if (line.isEmpty())
            continue;

        emit rawLineReceived(line);

        bool isNumber = false;
        int adcVal = line.toInt(&isNumber);

        if (isNumber && adcVal >= 0 && adcVal <= 4095) {
            emit dataReceived(adcVal);
        }
    }
}

void SerialManager::onErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        emit statusMessage(Logger::format(LogCategory::Serial, LogLevel::Error, "장치 연결이 예기치 않게 끊어졌습니다."), true);
        disconnectPort();
    }
}