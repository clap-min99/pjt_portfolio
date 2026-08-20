#ifndef CHARTMANAGER_H
#define CHARTMANAGER_H

#include <QObject>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

QT_CHARTS_USE_NAMESPACE

/**
 * @brief 실시간 가스 농도 곡선 및 동적 임계값 기준선을 렌더링하는 차트 매니저 클래스
 */
class ChartManager : public QObject {
    Q_OBJECT

public:
    explicit ChartManager(QObject* parent = nullptr);
    ~ChartManager();

    void initChart(QChartView* chartView);
    void addGasData(int value);
    void setThreshold(int threshold);

private:
    void updateThresholdLine();

    QChart* m_chart;
    QLineSeries* m_gasSeries;
    QLineSeries* m_thresholdSeries;
    QValueAxis* m_axisX;
    QValueAxis* m_axisY;

    int m_maxDataPoints;
    int m_dataIndex;
    int m_currentThreshold;
};

#endif // CHARTMANAGER_H