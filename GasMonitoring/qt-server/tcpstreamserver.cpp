#include "tcpstreamserver.h"
#include "logger.h"
#include <QDataStream>

TcpStreamServer::TcpStreamServer(QObject* parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_clientSocket(nullptr)
    , m_imageSize(0)
{
    connect(m_server, &QTcpServer::newConnection, this, &TcpStreamServer::onNewConnection);
}

TcpStreamServer::~TcpStreamServer()
{
    stopServer();
}

bool TcpStreamServer::startServer(quint16 port)
{
    if (m_server->listen(QHostAddress::AnyIPv4, port)) {
        emit logMessage(Logger::format(LogCategory::TCP, LogLevel::Info,
            QString("서버가 포트 %1에서 시작되었습니다.").arg(port)));
        return true;
    }

    emit logMessage(Logger::format(LogCategory::TCP, LogLevel::Error,
        QString("서버 시작 실패: %1").arg(m_server->errorString())));
    return false;
}

void TcpStreamServer::stopServer()
{
    if (m_clientSocket) {
        m_clientSocket->abort();
        m_clientSocket->deleteLater();
        m_clientSocket = nullptr;
        emit clientCountChanged(0);
    }

    m_buffer.clear();
    m_imageSize = 0;

    if (m_server->isListening()) {
        m_server->close();
    }
}

// 클라이언트로 실시간 가스 데이터 및 임계값 중계 (포맷: GAS:수치:임계값\n)
void TcpStreamServer::sendGasDataToClient(int adcValue, int threshold)
{
    if (m_clientSocket && m_clientSocket->isOpen()) {
        QString dataStr = QString("GAS:%1:%2\n").arg(adcValue).arg(threshold);
        m_clientSocket->write(dataStr.toUtf8());
    }
}

void TcpStreamServer::onNewConnection()
{
    if (m_clientSocket) {
        m_clientSocket->disconnect();
        m_clientSocket->abort();
        m_clientSocket->deleteLater();
        m_clientSocket = nullptr;
    }

    m_clientSocket = m_server->nextPendingConnection();
    connect(m_clientSocket, &QTcpSocket::readyRead, this, &TcpStreamServer::onReadyRead);
    connect(m_clientSocket, &QTcpSocket::disconnected, this, &TcpStreamServer::onClientDisconnected);

    m_buffer.clear();
    m_imageSize = 0;

    emit logMessage(Logger::format(LogCategory::TCP, LogLevel::Info,
        QString("클라이언트 연결됨 (%1)").arg(m_clientSocket->peerAddress().toString())));
    emit clientCountChanged(1);
}

// 가변 길이 바이너리(JPEG) 및 텍스트 제어 명령 분기 수신
void TcpStreamServer::onReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket || socket != m_clientSocket)
        return;

    m_buffer.append(socket->readAll());

    while (!m_buffer.isEmpty()) {
        if (m_imageSize == 0) {
            // 텍스트 제어 명령 처리 ('1': 차단, '0': 복구)
            if (static_cast<unsigned char>(m_buffer[0]) != 0x00) {
                int newlineIdx = m_buffer.indexOf('\n');
                if (newlineIdx != -1) {
                    QByteArray lineBytes = m_buffer.left(newlineIdx).trimmed();
                    m_buffer.remove(0, newlineIdx + 1);

                    QString cmd = QString::fromUtf8(lineBytes);
                    if (cmd == "1" || cmd == "0") {
                        emit valveCommandReceived(cmd.at(0).toLatin1());
                    }
                } else if (m_buffer.size() == 1 && (m_buffer[0] == '1' || m_buffer[0] == '0')) {
                    char cmdChar = m_buffer[0];
                    m_buffer.remove(0, 1);
                    emit valveCommandReceived(cmdChar);
                } else {
                    break;
                }
                continue;
            }

            // 비디오 프레임 4바이트 헤더(Big-Endian 페이로드 길이) 파싱
            if (m_buffer.size() < static_cast<int>(sizeof(qint32)))
                break;

            QDataStream stream(m_buffer.left(sizeof(qint32)));
            stream.setByteOrder(QDataStream::BigEndian);
            stream >> m_imageSize;

            m_buffer.remove(0, sizeof(qint32));

            // 비정상 패킷 크기 방어 로직 (최대 10MB)
            if (m_imageSize <= 0 || m_imageSize > 10 * 1024 * 1024) {
                emit logMessage(Logger::format(LogCategory::TCP, LogLevel::Warn,
                    QString("비정상 패킷 감지 (%1 Bytes). 버퍼를 초기화합니다.").arg(m_imageSize)));
                m_buffer.clear();
                m_imageSize = 0;
                return;
            }
        }

        // 이미지 바이너리 페이로드 수신
        if (m_buffer.size() < m_imageSize)
            break;

        QByteArray jpegData = m_buffer.left(m_imageSize);
        m_buffer.remove(0, m_imageSize);

        QPixmap pixmap;
        if (pixmap.loadFromData(jpegData, "JPEG")) {
            emit frameReceived(pixmap);
        }

        m_imageSize = 0;
    }
}

void TcpStreamServer::onClientDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    emit logMessage(Logger::format(LogCategory::TCP, LogLevel::Info, "클라이언트 연결이 해제되었습니다."));

    if (socket && socket == m_clientSocket) {
        m_clientSocket = nullptr;
        m_buffer.clear();
        m_imageSize = 0;
        emit clientCountChanged(0);
    }

    if (socket) {
        socket->deleteLater();
    }
}