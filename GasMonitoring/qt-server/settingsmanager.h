#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QString>

// 프로그램 전역 설정 저장 구조체
struct AppSettings {
    // [Safety]
    int gasThreshold = 3000; // 위험 경보 및 자동 차단 기준 가스 ADC 수치
    bool autoCloseEnabled = true; // 임계값 초과 시 자동 차단 활성화 여부

    // [Serial]
    QString serialPortName = ""; // 최근 연결 성공한 COM 포트명
    int serialBaudRate = 115200; // 시리얼 통신 보드레이트

    // [Network]
    int tcpPort = 8080; // 모바일 스트리밍 수신 TCP 포트

    // [Logging]
    bool csvSaveEnabled = true; // CSV 센서 데이터 자동 저장 여부
    QString csvLogPath = "./logs"; // CSV 파일 저장 디렉터리 경로
};

class SettingsManager {
public:
    explicit SettingsManager(const QString& fileName = "config.ini");
    ~SettingsManager() = default;

    AppSettings loadSettings();
    void saveSettings(const AppSettings& settings);

private:
    QString m_configFilePath;
};

#endif // SETTINGSMANAGER_H