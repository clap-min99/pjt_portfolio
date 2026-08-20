#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "chartmanager.h"
#include "logger.h"
#include "serialmanager.h"
#include "settingsmanager.h"
#include "tcpstreamserver.h"

#include <QCloseEvent>
#include <QEvent>
#include <QMainWindow>
#include <QPixmap>
#include <QString>
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

/**
 * @brief 스마트 모니터링 시스템의 메인 GUI 컨트롤러
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    // TCP 스트리밍 & 원격 제어 슬롯
    void on_btnStartServer_clicked();
    void onFrameReceived(const QPixmap& pixmap);
    void onClientCountChanged(int count);
    void onLogMessage(const QString& message);
    void onValveCommandFromClient(char cmd);

    // 시리얼 통신 & 밸브 제어 슬롯
    void on_btnSerialConnect_clicked();
    void on_btnValveClose_clicked();
    void on_btnValveOpen_clicked();
    void onGasDataReceived(int adcValue);
    void onSerialStatusMessage(const QString& msg, bool isError);
    void onSerialConnectionChanged(bool isConnected);

    // 설정 및 로깅 슬롯
    void on_btnApplyThreshold_clicked();
    void on_btnSelectPath_clicked();
    void on_btnOpenFolder_clicked();

private:
    QString getLocalIPAddress();
    void updateStatusBadge(bool isDanger);
    void logGasDataToCsv(int adcValue, int threshold, bool isDanger);
    void updatePortList();

    void loadAndApplySettings();
    void saveCurrentSettings();

private:
    Ui::MainWindow* ui;
    TcpStreamServer* m_streamServer;
    SerialManager* m_serialManager;
    ChartManager* m_chartManager;
    std::unique_ptr<SettingsManager> m_settingsManager;

    int m_currentThreshold = 3000;
    QString m_saveDirPath = "./logs";
    bool m_isValveClosed = false;
};

#endif // MAINWINDOW_H