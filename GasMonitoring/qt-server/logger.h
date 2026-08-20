#ifndef LOGGER_H
#define LOGGER_H

#include <QDateTime>
#include <QString>

/**
 * @brief 시스템 로깅 카테고리 분류
 */
enum class LogCategory {
    TCP,
    Serial,
    System
};

/**
 * @brief 로그 심각도 및 송수신 상태 레벨
 */
enum class LogLevel {
    Info,
    Warn,
    Error,
    Tx,
    Rx
};

/**
 * @brief 시스템 전역 로그 메시지 포맷팅을 담당하는 유틸리티 클래스
 */
class Logger {
public:
    // 정적 유틸리티 클래스로 인스턴스화 방지
    Logger() = delete;

    /**
     * @brief 타임스탬프와 카테고리, 로그 레벨이 포함된 표준 로그 문자열 생성
     * @return [hh:mm:ss] [CATEGORY] [LEVEL] 메시지 형태의 포맷 문자열
     */
    static QString format(LogCategory category, LogLevel level, const QString& message)
    {
        QString timeStr = QDateTime::currentDateTime().toString("hh:mm:ss");
        return QString("[%1] [%2] [%3] %4")
            .arg(timeStr)
            .arg(categoryToString(category))
            .arg(levelToString(level))
            .arg(message);
    }

    /**
     * @brief 기본 System 카테고리로 로그 문자열을 생성하는 오버로딩 함수
     */
    static QString format(LogLevel level, const QString& message)
    {
        return format(LogCategory::System, level, message);
    }

private:
    static const char* categoryToString(LogCategory category)
    {
        switch (category) {
        case LogCategory::TCP:
            return "TCP";
        case LogCategory::Serial:
            return "SERIAL";
        case LogCategory::System:
            return "SYSTEM";
        }
        return "UNKNOWN";
    }

    static const char* levelToString(LogLevel level)
    {
        switch (level) {
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warn:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
        case LogLevel::Tx:
            return "TX";
        case LogLevel::Rx:
            return "RX";
        }
        return "INFO";
    }
};

#endif // LOGGER_H