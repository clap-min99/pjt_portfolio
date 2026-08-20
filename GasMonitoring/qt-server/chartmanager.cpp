#include "chartmanager.h"
#include <QPen>

ChartManager::ChartManager(QObject* parent)
    : QObject(parent)
    , m_chart(new QChart())
    , m_gasSeries(new QLineSeries())
    , m_thresholdSeries(new QLineSeries())
    , m_axisX(new QValueAxis())
    , m_axisY(new QValueAxis())
    , m_maxDataPoints(50)
    , m_dataIndex(0)
    , m_currentThreshold(3000)
{
}

ChartManager::~ChartManager()
{
}

void ChartManager::initChart(QChartView* chartView)
{
    if (!chartView)
        return;

    m_chart->setTitle("실시간 가스 수치 추이");
    m_chart->legend()->setVisible(true);
    m_chart->legend()->setAlignment(Qt::AlignBottom);

    // 가스 수치 곡선 (파란 실선)
    m_gasSeries->setName("가스 수치");
    QPen gasPen(QColor(0, 122, 255));
    gasPen.setWidth(2);
    m_gasSeries->setPen(gasPen);

    // 임계값 기준선 (빨간 점선)
    m_thresholdSeries->setName("위험 임계값");
    QPen thresholdPen(Qt::red);
    thresholdPen.setWidth(2);
    thresholdPen.setStyle(Qt::DashLine);
    m_thresholdSeries->setPen(thresholdPen);

    m_chart->addSeries(m_gasSeries);
    m_chart->addSeries(m_thresholdSeries);

    // 축 범위 및 속성 정의 (12-bit ADC: 0 ~ 4095)
    m_axisX->setRange(0, m_maxDataPoints);
    m_axisX->setLabelFormat("%d");
    m_axisX->setTitleText("샘플");

    m_axisY->setRange(0, 4095);
    m_axisY->setTitleText("Gas Value");

    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_chart->addAxis(m_axisY, Qt::AlignLeft);

    m_gasSeries->attachAxis(m_axisX);
    m_gasSeries->attachAxis(m_axisY);
    m_thresholdSeries->attachAxis(m_axisX);
    m_thresholdSeries->attachAxis(m_axisY);

    chartView->setChart(m_chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    updateThresholdLine();
}

// 실시간 슬라이딩 윈도우 방식으로 데이터 포인트 갱신
void ChartManager::addGasData(int value)
{
    m_gasSeries->append(m_dataIndex, value);

    if (m_dataIndex > m_maxDataPoints) {
        m_gasSeries->remove(0);
        m_axisX->setRange(m_dataIndex - m_maxDataPoints, m_dataIndex);
    } else {
        m_axisX->setRange(0, m_maxDataPoints);
    }

    m_dataIndex++;
    updateThresholdLine();
}

void ChartManager::setThreshold(int threshold)
{
    m_currentThreshold = threshold;
    updateThresholdLine();
}

// 차트 X축 스크롤에 맞추어 임계값 기준선 양 끝 좌표 갱신
void ChartManager::updateThresholdLine()
{
    m_thresholdSeries->clear();
    m_thresholdSeries->append(m_axisX->min(), m_currentThreshold);
    m_thresholdSeries->append(m_axisX->max(), m_currentThreshold);
}