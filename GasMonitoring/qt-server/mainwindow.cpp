#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QNetworkInterface>
#include <QSignalBlocker>
#include <QTextStream>
#include <QUrl>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_streamServer(new TcpStreamServer(this))
    , m_serialManager(new SerialManager(this))
    , m_chartManager(new ChartManager(this))
    , m_settingsManager(std::make_unique<SettingsManager>("config.ini"))
    , m_currentThreshold(3000)
    , m_saveDirPath("./logs")
    , m_isValveClosed(false)
{
    ui->setupUi(this);

    // UI 기본 위젯 초기화
    if (ui->lblServerIp) {
        ui->lblServerIp->setText(QString("IP: %1").arg(getLocalIPAddress()));
    }
    if (ui->lblClientCount) {
        ui->lblClientCount->setText("접속 상태: 클라이언트 0 명 접속 중");
    }
    if (ui->cbBaudRate) {
        ui->cbBaudRate->clear();
        ui->cbBaudRate->addItems({ "9600", "19200", "38400", "57600", "115200" });
        ui->cbBaudRate->setCurrentText("115200");
    }
    if (ui->cbPortList) {
        ui->cbPortList->installEventFilter(this);
        updatePortList();
    }

    // TCP 스트림 서버 시그널 연동
    connect(m_streamServer, &TcpStreamServer::frameReceived, this, &MainWindow::onFrameReceived);
    connect(m_streamServer, &TcpStreamServer::logMessage, this, &MainWindow::onLogMessage);
    connect(m_streamServer, &TcpStreamServer::clientCountChanged, this, &MainWindow::onClientCountChanged);
    connect(m_streamServer, &TcpStreamServer::valveCommandReceived, this, &MainWindow::onValveCommandFromClient);

    // 시리얼 통신 모듈 시그널 연동
    connect(m_serialManager, &SerialManager::dataReceived, this, &MainWindow::onGasDataReceived);
    connect(m_serialManager, &SerialManager::statusMessage, this, &MainWindow::onSerialStatusMessage);
    connect(m_serialManager, &SerialManager::connectionStateChanged, this, &MainWindow::onSerialConnectionChanged);

    // 실시간 차트 및 임계값 컨트롤 초기화
    if (ui->chartView) {
        m_chartManager->initChart(ui->chartView);
        m_chartManager->setThreshold(m_currentThreshold);
    }
    if (ui->sbThreshold) {
        ui->sbThreshold->setValue(m_currentThreshold);
        connect(ui->sbThreshold, &QSpinBox::editingFinished, this, &MainWindow::on_btnApplyThreshold_clicked);
    }

    updateStatusBadge(false);
    loadAndApplySettings();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    saveCurrentSettings();
    event->accept();
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    // 콤보박스 클릭 시 미연결 상태일 때만 포트 목록을 동적으로 재스캔
    if (watched == ui->cbPortList && event->type() == QEvent::MouseButtonPress) {
        if (m_serialManager && !m_serialManager->isConnected()) {
            updatePortList();
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::updatePortList()
{
    if (!ui->cbPortList || !m_serialManager) {
        return;
    }

    // UI 갱신 중 불필요한 currentIndexChanged 시그널 방지
    const QSignalBlocker blocker(ui->cbPortList);

    const QString currentText = ui->cbPortList->currentText();
    ui->cbPortList->clear();

    QStringList ports = m_serialManager->availablePorts();

    // 인식된 포트가 없을 경우 기본 가상 포트(COM1~COM10) 목록 생성
    if (ports.isEmpty()) {
        for (int i = 1; i <= 10; ++i) {
            ports.append(QString("COM%1").arg(i));
        }
    }

    ui->cbPortList->addItems(ports);

    // 이전 선택 포트가 새 목록에 존재하면 선택 유지
    int idx = ui->cbPortList->findText(currentText);
    if (idx != -1) {
        ui->cbPortList->setCurrentIndex(idx);
    }
}

void MainWindow::loadAndApplySettings()
{
    AppSettings settings = m_settingsManager->loadSettings();

    m_currentThreshold = settings.gasThreshold;
    if (ui->sbThreshold) {
        ui->sbThreshold->setValue(m_currentThreshold);
    }
    if (m_chartManager) {
        m_chartManager->setThreshold(m_currentThreshold);
    }
    if (ui->chkAutoClose) {
        ui->chkAutoClose->setChecked(settings.autoCloseEnabled);
    }

    if (ui->lePort) {
        ui->lePort->setText(QString::number(settings.tcpPort));
    }

    if (ui->cbBaudRate) {
        ui->cbBaudRate->setCurrentText(QString::number(settings.serialBaudRate));
    }
    if (ui->cbPortList && !settings.serialPortName.isEmpty()) {
        int idx = ui->cbPortList->findText(settings.serialPortName);
        if (idx != -1) {
            ui->cbPortList->setCurrentIndex(idx);
        }
    }

    m_saveDirPath = settings.csvLogPath;
    if (ui->leLogPath) {
        ui->leLogPath->setText(m_saveDirPath);
    }
    if (ui->chkEnableLogging) {
        ui->chkEnableLogging->setChecked(settings.csvSaveEnabled);
    }
}

void MainWindow::saveCurrentSettings()
{
    AppSettings settings;

    settings.gasThreshold = m_currentThreshold;
    settings.autoCloseEnabled = ui->chkAutoClose ? ui->chkAutoClose->isChecked() : true;

    settings.serialPortName = ui->cbPortList ? ui->cbPortList->currentText() : "";
    settings.serialBaudRate = ui->cbBaudRate ? ui->cbBaudRate->currentText().toInt() : 115200;

    settings.tcpPort = ui->lePort ? ui->lePort->text().toInt() : 8080;

    settings.csvSaveEnabled = ui->chkEnableLogging ? ui->chkEnableLogging->isChecked() : true;
    settings.csvLogPath = m_saveDirPath;

    m_settingsManager->saveSettings(settings);
}

QString MainWindow::getLocalIPAddress()
{
    const QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress& address : addresses) {
        if (address != QHostAddress::LocalHost && address.toIPv4Address()) {
            return address.toString();
        }
    }
    return "127.0.0.1";
}

void MainWindow::on_btnStartServer_clicked()
{
    if (ui->btnStartServer->text() == "서버 시작") {
        quint16 port = ui->lePort ? ui->lePort->text().toUShort() : 8080;
        if (port == 0) {
            port = 8080;
        }

        if (m_streamServer->startServer(port)) {
            ui->btnStartServer->setText("서버 중지");
            if (ui->lePort) {
                ui->lePort->setEnabled(false);
            }
        }
    } else {
        m_streamServer->stopServer();
        ui->btnStartServer->setText("서버 시작");
        if (ui->lePort) {
            ui->lePort->setEnabled(true);
        }
        onClientCountChanged(0);
    }
}

void MainWindow::onClientCountChanged(int count)
{
    if (ui->lblClientCount) {
        ui->lblClientCount->setText(QString("접속 상태: 클라이언트 %1 명 접속 중").arg(count));
    }
}

void MainWindow::onFrameReceived(const QPixmap& pixmap)
{
    if (ui->lblCameraPreview) {
        ui->lblCameraPreview->setPixmap(
            pixmap.scaled(ui->lblCameraPreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void MainWindow::onLogMessage(const QString& message)
{
    if (ui->teLog) {
        ui->teLog->append(message);
    }
}

void MainWindow::on_btnSerialConnect_clicked()
{
    if (m_serialManager->isConnected()) {
        m_serialManager->disconnectPort();
    } else {
        QString selectedPort = ui->cbPortList ? ui->cbPortList->currentText() : "";
        qint32 baudRate = ui->cbBaudRate ? ui->cbBaudRate->currentText().toInt() : 115200;

        if (!selectedPort.isEmpty()) {
            m_serialManager->connectPort(selectedPort, baudRate);
        }
    }
}

void MainWindow::on_btnValveClose_clicked()
{
    if (m_serialManager->sendChar('1')) {
        m_isValveClosed = true;
        onLogMessage(Logger::format(LogCategory::Serial, LogLevel::Tx, "STM32로 밸브 차단 명령('1') 전송"));
    }
}

void MainWindow::on_btnValveOpen_clicked()
{
    if (m_serialManager->sendChar('0')) {
        m_isValveClosed = false;
        onLogMessage(Logger::format(LogCategory::Serial, LogLevel::Tx, "STM32로 밸브 복구 명령('0') 전송"));
    }
}

void MainWindow::onGasDataReceived(int adcValue)
{
    if (ui->lblGasVal) {
        ui->lblGasVal->setText(QString::number(adcValue));
    }

    if (m_chartManager) {
        m_chartManager->addGasData(adcValue);
    }

    if (m_streamServer) {
        m_streamServer->sendGasDataToClient(adcValue, m_currentThreshold);
    }

    bool isDanger = (adcValue >= m_currentThreshold);
    logGasDataToCsv(adcValue, m_currentThreshold, isDanger);
    updateStatusBadge(isDanger);

    // 임계값 초과 시 단일 트리거 래치 기반 자동 차단
    if (ui->chkAutoClose && ui->chkAutoClose->isChecked() && isDanger) {
        if (!m_isValveClosed) {
            if (m_serialManager->sendChar('1')) {
                m_isValveClosed = true;
                onLogMessage(Logger::format(LogCategory::Serial, LogLevel::Warn,
                    QString("임계값 초과 감지 (%1 >= %2). 밸브를 자동으로 차단합니다.")
                        .arg(adcValue)
                        .arg(m_currentThreshold)));
            }
        }
    }
}

void MainWindow::onValveCommandFromClient(char cmd)
{
    if (cmd == '1') {
        onLogMessage(Logger::format(LogCategory::TCP, LogLevel::Info, "안드로이드 원격 제어: 밸브 차단 요청"));
        on_btnValveClose_clicked();
    } else if (cmd == '0') {
        onLogMessage(Logger::format(LogCategory::TCP, LogLevel::Info, "안드로이드 원격 제어: 밸브 복구 요청"));
        on_btnValveOpen_clicked();
    }
}

void MainWindow::on_btnApplyThreshold_clicked()
{
    if (!ui->sbThreshold) {
        return;
    }

    int inputVal = ui->sbThreshold->value();
    if (m_currentThreshold == inputVal) {
        return;
    }

    m_currentThreshold = inputVal;

    if (m_chartManager) {
        m_chartManager->setThreshold(m_currentThreshold);
    }

    onLogMessage(Logger::format(LogCategory::System, LogLevel::Info,
        QString("가스 위험 임계값 변경 적용: %1 ADC").arg(m_currentThreshold)));
}

void MainWindow::on_btnSelectPath_clicked()
{
    QString selectedDir = QFileDialog::getExistingDirectory(
        this, "로그 파일 저장 폴더 선택", m_saveDirPath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!selectedDir.isEmpty()) {
        m_saveDirPath = selectedDir;
        if (ui->leLogPath) {
            ui->leLogPath->setText(m_saveDirPath);
        }
        onLogMessage(Logger::format(LogCategory::System, LogLevel::Info,
            QString("CSV 저장 경로 변경: %1").arg(m_saveDirPath)));
    }
}

void MainWindow::on_btnOpenFolder_clicked()
{
    QDir dir(m_saveDirPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_saveDirPath));
}

void MainWindow::logGasDataToCsv(int adcValue, int threshold, bool isDanger)
{
    if (ui->chkEnableLogging && !ui->chkEnableLogging->isChecked()) {
        return;
    }

    QDir dir(m_saveDirPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString dateStr = QDate::currentDate().toString("yyyy-MM-dd");
    QString filePath = QString("%1/gas_log_%2.csv").arg(m_saveDirPath, dateStr);

    QFile file(filePath);
    bool isNewFile = !file.exists();

    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);

        if (isNewFile) {
            out << "Timestamp,ADC_Value,Threshold,Status\n";
        }

        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        QString status = isDanger ? "DANGER" : "NORMAL";

        out << timestamp << "," << adcValue << "," << threshold << "," << status << "\n";
        file.close();
    }
}

void MainWindow::updateStatusBadge(bool isDanger)
{
    if (!ui->lblStatusBadge) {
        return;
    }

    if (isDanger) {
        ui->lblStatusBadge->setText("위 험");
        ui->lblStatusBadge->setStyleSheet("background-color: #FF2D55; color: white; font-weight: bold; border-radius: 4px;");
    } else {
        ui->lblStatusBadge->setText("정 상");
        ui->lblStatusBadge->setStyleSheet("background-color: #34C759; color: white; font-weight: bold; border-radius: 4px;");
    }
}

void MainWindow::onSerialStatusMessage(const QString& msg, bool isError)
{
    Q_UNUSED(isError);
    onLogMessage(msg);
}

void MainWindow::onSerialConnectionChanged(bool isConnected)
{
    if (ui->btnSerialConnect) {
        ui->btnSerialConnect->setText(isConnected ? "시리얼 해제" : "시리얼 연결");
    }
    if (ui->cbPortList) {
        ui->cbPortList->setEnabled(!isConnected);
    }
    if (ui->cbBaudRate) {
        ui->cbBaudRate->setEnabled(!isConnected);
    }
}