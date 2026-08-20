#ifndef TCPSTREAMSERVER_H
#define TCPSTREAMSERVER_H

#include <QByteArray>
#include <QObject>
#include <QPixmap>
#include <QTcpServer>
#include <QTcpSocket>

/**
 * @brief 안드로이드 클라이언트와의 영상 스트리밍 및 원격 제어 명령 중계를 담당하는 TCP 서버 클래스
 */
class TcpStreamServer : public QObject {
    Q_OBJECT
public:
    explicit TcpStreamServer(QObject* parent = nullptr);
    ~TcpStreamServer();

    bool startServer(quint16 port);
    void stopServer();
    void sendGasDataToClient(int adcValue, int threshold);

signals:
    void frameReceived(const QPixmap& pixmap);
    void logMessage(const QString& message);
    void clientCountChanged(int count);
    void valveCommandReceived(char cmd);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onClientDisconnected();

private:
    QTcpServer* m_server;
    QTcpSocket* m_clientSocket;
    QByteArray m_buffer;
    qint32 m_imageSize;
};

#endif // TCPSTREAMSERVER_H