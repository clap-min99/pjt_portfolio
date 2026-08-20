#include "settingsmanager.h"

SettingsManager::SettingsManager(const QString& fileName)
{
    // 실행 파일 디렉터리 기준으로 config.ini 경로 생성
    m_configFilePath = QDir(QCoreApplication::applicationDirPath()).filePath(fileName);
}

AppSettings SettingsManager::loadSettings()
{
    AppSettings settings;
    QSettings ini(m_configFilePath, QSettings::IniFormat);

    // 1. [Safety] 설정 로드
    ini.beginGroup("Safety");
    settings.gasThreshold = ini.value("gas_threshold", settings.gasThreshold).toInt();
    settings.autoCloseEnabled = ini.value("auto_close_enabled", settings.autoCloseEnabled).toBool();
    ini.endGroup();

    // 2. [Serial] 설정 로드
    ini.beginGroup("Serial");
    settings.serialPortName = ini.value("port_name", settings.serialPortName).toString();
    settings.serialBaudRate = ini.value("baud_rate", settings.serialBaudRate).toInt();
    ini.endGroup();

    // 3. [Network] 설정 로드
    ini.beginGroup("Network");
    settings.tcpPort = ini.value("tcp_port", settings.tcpPort).toInt();
    ini.endGroup();

    // 4. [Logging] 설정 로드
    ini.beginGroup("Logging");
    settings.csvSaveEnabled = ini.value("csv_save_enabled", settings.csvSaveEnabled).toBool();
    settings.csvLogPath = ini.value("csv_log_path", settings.csvLogPath).toString();
    ini.endGroup();

    // 비정상 수치 입력 방지를 위한 기본값 보정
    if (settings.gasThreshold <= 0 || settings.gasThreshold > 4095)
        settings.gasThreshold = 3000;
    if (settings.tcpPort <= 0 || settings.tcpPort > 65535)
        settings.tcpPort = 8080;

    return settings;
}

void SettingsManager::saveSettings(const AppSettings& settings)
{
    QSettings ini(m_configFilePath, QSettings::IniFormat);

    // 1. [Safety] 설정 저장
    ini.beginGroup("Safety");
    ini.setValue("gas_threshold", settings.gasThreshold);
    ini.setValue("auto_close_enabled", settings.autoCloseEnabled);
    ini.endGroup();

    // 2. [Serial] 설정 저장
    ini.beginGroup("Serial");
    ini.setValue("port_name", settings.serialPortName);
    ini.setValue("baud_rate", settings.serialBaudRate);
    ini.endGroup();

    // 3. [Network] 설정 저장
    ini.beginGroup("Network");
    ini.setValue("tcp_port", settings.tcpPort);
    ini.endGroup();

    // 4. [Logging] 설정 저장
    ini.beginGroup("Logging");
    ini.setValue("csv_save_enabled", settings.csvSaveEnabled);
    ini.setValue("csv_log_path", settings.csvLogPath);
    ini.endGroup();

    ini.sync(); // 디스크 즉시 동기화
}