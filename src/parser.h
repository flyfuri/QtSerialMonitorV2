#ifndef PARSER_H
#define PARSER_H

#include <QObject>
#include <QTime>
#include <QElapsedTimer>
#include <QDebug>
#include <QTimer>
#include <QApplication>
#include <QRegularExpression>
#include <QThread>
#include <QRunnable>
#include <QEventLoop>
#include <QMutex>
#include <QSemaphore>
#include <QQueue>
#include <QDebug>
#include "serial.h"
#include "qcustomplot.h"

#define MS_PER_DAY 86400000l


namespace PARSER{

enum PARSER_PRE_FILTER_MODE
{
    None = 0,
    trimmed,
    simplified
};

struct ParsedData{
    QSharedPointer<QMutex> mutex = nullptr;
    QSharedPointer<QSemaphore> semaphorGUIprio; //gui signals it needs priority using semaphore(1) as a mutex to use available()...
    QSharedPointer<QQueue<QString>> queueLabels = nullptr;
    QSharedPointer<QQueue<double>> queueNumericData = nullptr;
    QSharedPointer<QQueue<long>> queueTimeStamps = nullptr;
    QSharedPointer<QString> strqueuePrefiltered = nullptr; //to pass on inputString prefiltered (simplified, stripped or nothing)
    QSharedPointer<QQueue<QString>> queueChartGraphLabels = nullptr;
    QSharedPointer<QQueue<QSharedPointer<QVector<QCPGraphData>>>> queueChartGraphs= nullptr;
};

class Parser : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit Parser(QObject *parent = nullptr);
    explicit Parser(ParsedData parsedDataPointers, QObject *parent = nullptr);
    QList<double> getDataStorage();
    QList<double> getListNumericValues();
    QList<long> getListTimeStamp();
    QList<long> getTimeStorage();
    QStringList getLabelStorage();
    QStringList getStringListLabels();
    QStringList getStringListNumericData();
    QStringList getTextList();
    QStringList searchTimeFormatList = {"hh:mm:ss:zzz", "hh:mm:ss.zzz", "hh:mm:ss.z", "hh:mm:ss"};
    void abort();
    void appendSetToMemory(QStringList newlabelList, QList<double> newDataList, QList<long> newTimeList, QString text = "");
    void clear();
    void clearExternalClock();
    void clearStorage();
    void parse(QString inputString, QString externalClockLabel, serial::SERIAL_TSTAMP_MODE tstampmode=serial::NoTStamp,
               int fixinterval = 2, double timebase_s = 0.001);
    void parseCSV(QString inputString, bool useExternalLabel = false, QString externalClockLabel = "");
    void parserClockAddMSecs(int millis);
    void resetTimeRange();
    void restartChartTimer();
    void setParsingTimeRange(QTime minTime, QTime maxTime);
    void setReportProgress(bool isEnabled);
    void getCSVReadyData(QStringList *columnNames, QList<QList<double> > *dataColumns);

    static long timeTomsSinceBeginOfDay(QTime);
    void setParseSettings(int tstampMode=-1, QString extClockLabel="", int fixintv=-1, double tbase_s=-1, int prefltr=-1);
    void run(); // QRunnable interface
signals:
    void updateProgress(float *percent);
    void finished();
public slots:
    void parseSlot(QString inputString);
    void finish();
private:
    bool abortFlag = false;
    bool canReportProgress = false;
    float parsingProgressPercent = 0.0f;
    int lineCount = 0;
    struct ParsSettings{
        serial::SERIAL_TSTAMP_MODE timestampMode = serial::NoTStamp;
        QString externalClockLabel;
        int fixinterval;
        double timebase_s;
        PARSER_PRE_FILTER_MODE prefilterMode = None;
    } parsSettings;
    ParsedData parseddata;
    QList<double> dataStorage;
    QList<double> listNumericData;
    QList<long> listTimeStamp;
    QList<long> timeStampStorage;
    QStringList labelStorage;
    QStringList stringListNumericData, stringListLabels;
    QStringList textStorage;
    QElapsedTimer parserTimer;
    QTime parserClock;
    long latestTimeStamp_ms;
    QTime minimumTime, maximumTime;
    QMutex mutex;
};

} //end namespace

#endif // PARSER_H
