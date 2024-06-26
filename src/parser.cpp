#include "parser.h"

namespace PARSER{

Parser::Parser(QObject *parent) : QObject(parent)
{
    //ToDo why originally on the heap? parserTimer = new QElapsedTimer;
    //ToDo why originally on the heap?  parserClock = new QTime;
    parserTimer.start();
    latestTimeStamp_ms = 0; //setHMS(0, 0, 0, 0);
}

Parser::Parser(ParsedData parsedDataPointers, QObject *parent)
{
    if(parsedDataPointers.mutex != nullptr && parsedDataPointers.queueLabels != nullptr && parsedDataPointers.queueNumericData != nullptr
        && parsedDataPointers.queueTimeStamps != nullptr && parsedDataPointers.strqueuePrefiltered != nullptr)
    {
        parseddata = parsedDataPointers;
    }
    parserTimer.start();
    latestTimeStamp_ms = 0; //setHMS(0, 0, 0, 0);
}


void Parser::run()
{
    QScopedPointer<QEventLoop> parsloop(new QEventLoop);
    connect(this, &Parser::finished, parsloop.data(), &QEventLoop::quit);
    parsloop->exec();
}

void Parser::parseSlot(QString inputString)
{
    //old style
    listNumericData.clear();
    stringListLabels.clear();
    listTimeStamp.clear();
    lineCount = 0;

    //new style
    QList<QString> graphLabels;  // list with unique graph labels
    QList<QSharedPointer<QVector<QCPGraphData>>> chartGraphsData;  //list data for all graphs in a form it can diectly be added to chart.

    QStringList inputStringSplitArrayLines = inputString.split(QRegularExpression("[\\n+\\r+]"), Qt::SplitBehaviorFlags::SkipEmptyParts);
    lineCount = inputStringSplitArrayLines.count();

    for (auto l = 0; l < inputStringSplitArrayLines.count(); ++l)
    {
        parsingProgressPercent = (float)l / inputStringSplitArrayLines.count() * 100.0F;
        if (l % 50 == 0 && canReportProgress)
        {
            emit updateProgress(&parsingProgressPercent);
            QApplication::processEvents(); // Prevents app for freeezeng during processing large files and allows to udpate progress percent. A very cheap trick...
        }

        if (abortFlag)
        {
            abortFlag = false;
            break;
        }

        QRegularExpression mainSymbols("^[-+]?[0-9]*\\.?[0-9]+$"); // float only   //  QRegularExpression mainSymbols("[-+]?[0-9]*\.?[0-9]+");
        QRegularExpression alphanumericSymbols("\\w+");
        QRegularExpression sepSymbols("[\t=,;]");

        inputStringSplitArrayLines[l].replace(sepSymbols, " ");
        QStringList inputStringSplitArray = inputStringSplitArrayLines[l].simplified().split(QRegularExpression("\\s+"), Qt::SplitBehaviorFlags::SkipEmptyParts); // rozdzielamy traktująac spacje jako separator

        if (parsSettings.timestampMode == serial::SysTimeStamp && inputStringSplitArray.last().contains("RDTSTAMP")){
            long tmpms = inputStringSplitArray.last().replace("RDTSTAMP", "").toInt();
            QTime tmpTime = QTime::fromMSecsSinceStartOfDay(tmpms);
            if(tmpTime.isValid())
            {
                latestTimeStamp_ms = tmpms;
            }
            else
            {
                latestTimeStamp_ms += parsSettings.fixinterval; //ToDo find out whats best when timestamp tag not present..
                if(latestTimeStamp_ms > MS_PER_DAY){latestTimeStamp_ms -= MS_PER_DAY;}//ToDo micros
            }
            inputStringSplitArray.removeLast();
        }
        else if (parsSettings.timestampMode == serial::ExternalTStamp)  //ToDo time format as intervals...
        {
            for (auto i = 0; i < inputStringSplitArray.count(); ++i)
            {
                if (parsSettings.externalClockLabel.isEmpty() == false && inputStringSplitArray[i] == parsSettings.externalClockLabel
                    && (QTime::fromMSecsSinceStartOfDay((inputStringSplitArray[i + 1]).toInt()).isValid()))
                {
                    latestTimeStamp_ms = inputStringSplitArray[i + 1].toInt();
                }
                else if (parsSettings.externalClockLabel.isEmpty() == true)
                {
                    foreach (auto timeFormat, searchTimeFormatList)
                    {
                        if (QTime::fromString(inputStringSplitArray[i], timeFormat).isValid())
                        {
                            latestTimeStamp_ms = this->timeTomsSinceBeginOfDay(QTime::fromString(inputStringSplitArray[i], timeFormat));
                            break;
                        }
                    }


                }
            }

        }
        else if (parsSettings.timestampMode == serial::FixIntervalTStamp)
        {
            latestTimeStamp_ms += parsSettings.fixinterval;
            if(latestTimeStamp_ms > MS_PER_DAY){latestTimeStamp_ms -= MS_PER_DAY;}//ToDo micros
        }
        else //tstampmode == serial::NoTStamp
        {
            listTimeStamp.append(parserTimer.elapsed());
        }



        for (auto i = 0; i < inputStringSplitArray.count(); ++i)
        {
            if (parsSettings.timestampMode == serial::ExternalTStamp)
            {
                if (minimumTime != QTime(0, 0, 0) && maximumTime != QTime(0, 0, 0))
                {
                    if (QTime::fromMSecsSinceStartOfDay(latestTimeStamp_ms) < minimumTime
                        || QTime::fromMSecsSinceStartOfDay(latestTimeStamp_ms) > maximumTime)
                    {
                        continue;
                    }
                }
            }

            // Labels +
            if (i == 0 && mainSymbols.match(inputStringSplitArray[0]).hasMatch())
            {
                //old style
                listNumericData.append(inputStringSplitArray[i].toDouble());
                stringListLabels.append("Graph 0");
            }
            else if (i > 0 && mainSymbols.match(inputStringSplitArray[i]).hasMatch() && mainSymbols.match(inputStringSplitArray[i - 1]).hasMatch())
            {
                //old style
                listNumericData.append(inputStringSplitArray[i].toDouble());
                stringListLabels.append("Graph " + QString::number(i));
            }
            else if (i > 0 && mainSymbols.match(inputStringSplitArray[i]).hasMatch() && !mainSymbols.match(inputStringSplitArray[i - 1]).hasMatch())
            {
                //old style
                listNumericData.append(inputStringSplitArray[i].toDouble());
                stringListLabels.append(inputStringSplitArray[i - 1]);
            }
            else
            {
                continue; // We didnt find or add any new data points so lets not log time and skip to the next element on the list...
            }

            //old style
            listTimeStamp.append(latestTimeStamp_ms);
            //new style
            QString tmplabel = stringListLabels.last();
            QCPGraphData tmpdata_with_time;
            tmpdata_with_time.key = latestTimeStamp_ms * parsSettings.timebase_s;
            tmpdata_with_time.value = listNumericData.last();
            int tmplabelindex = graphLabels.indexOf(tmplabel);
            if(tmplabelindex < 0)
            {
                graphLabels.append(tmplabel);
                QSharedPointer<QVector<QCPGraphData>> tmpshardptr(new QVector<QCPGraphData>());
                tmpshardptr->append(tmpdata_with_time);
                chartGraphsData.append(tmpshardptr);
            }
            else
            {
               chartGraphsData[tmplabelindex]->append(tmpdata_with_time);
            }
        }
    }
    if (parseddata.mutex != nullptr) //ToDo would be nice to ensure all lists are the same length
    {
        while(parseddata.semaphorGUIprio->available() <= 0){} //wait if GUI needs to be prioritized

        QMutexLocker<QMutex> locker(parseddata.mutex.data());     //lock Mutex for queue_________________________________________________

        //new style
        bool addToExistingQueueRecord = true;
        foreach(const QString graphlabel, graphLabels)
        {
            if(parseddata.queueChartGraphLabels->indexOf(graphlabel) < 0)
            {
                addToExistingQueueRecord = false;
                break;
            }
        }
        if(addToExistingQueueRecord)
        {
           foreach(const QString graphlabel, graphLabels)
            {
                QSharedPointer<QVector<QCPGraphData>> ptr_list_vec(chartGraphsData.at(graphLabels.lastIndexOf(graphlabel)));
                QSharedPointer<QVector<QCPGraphData>> ptr_queue_vec(parseddata.queueChartGraphs->at(parseddata.queueChartGraphLabels->lastIndexOf(graphlabel)));
                foreach(QCPGraphData vec, *ptr_list_vec)
                {
                    ptr_queue_vec->append(vec);
                }
            }
        }
        else
        {
            foreach(const QString graphlabel, graphLabels)
            {
                parseddata.queueChartGraphLabels->enqueue(graphlabel);
            }
            foreach(const QSharedPointer<QVector<QCPGraphData>> graphdata, chartGraphsData)
            {
                parseddata.queueChartGraphs->enqueue(graphdata);
            }
        }

        //old style
        foreach(const QString &label, stringListLabels)
        {
            parseddata.queueLabels->enqueue(label);
        }
        foreach(const double &record, listNumericData)
        {
            parseddata.queueNumericData->enqueue(record);
        }
        foreach(const long &tstamp, listTimeStamp)
        {
            parseddata.queueTimeStamps->enqueue(tstamp);
        }
        if(parsSettings.prefilterMode == None)
            parseddata.strqueuePrefiltered->append(inputString);
        else if(parsSettings.prefilterMode == simplified)
            parseddata.strqueuePrefiltered->append(inputString.simplified());
        else if(parsSettings.prefilterMode == trimmed)
            parseddata.strqueuePrefiltered->append(inputString.trimmed());

        //qDebug() << Q_FUNC_INFO << "write to queue" << QThread::currentThread() << parseddata.queueLabels->count();
    }  //unlock Mutex for queue____________________________________________________________________________________________________________
}

void Parser::finish()
{
    emit finished();
}

void Parser::parse(QString inputString, QString externalClockLabel, serial::SERIAL_TSTAMP_MODE tstampmode,
                   int fixinterval, double timebase_s)
{
    //    \s Matches a whitespace character (QChar::isSpace()).
    //    QStringList list = str.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    //    QStringList list = str.split(QRegExp("[\r\n\t ]+"), QString::SkipEmptyParts);

    listNumericData.clear();
    stringListLabels.clear();
    listTimeStamp.clear();
    lineCount = 0;

    QStringList inputStringSplitArrayLines = inputString.split(QRegularExpression("[\\n+\\r+]"), Qt::SplitBehaviorFlags::SkipEmptyParts);
    lineCount = inputStringSplitArrayLines.count();

    for (auto l = 0; l < inputStringSplitArrayLines.count(); ++l)
    {
        parsingProgressPercent = (float)l / inputStringSplitArrayLines.count() * 100.0F;
        if (l % 50 == 0 && canReportProgress)
        {
            emit updateProgress(&parsingProgressPercent);
            QApplication::processEvents(); // Prevents app for freeezeng during processing large files and allows to udpate progress percent. A very cheap trick...
        }

        if (abortFlag)
        {
            abortFlag = false;
            break;
        }

        QRegularExpression mainSymbols("^[-+]?[0-9]*\\.?[0-9]+$"); // float only   //  QRegularExpression mainSymbols("[-+]?[0-9]*\.?[0-9]+");
        QRegularExpression alphanumericSymbols("\\w+");
        QRegularExpression sepSymbols("[\t=,;]");

        inputStringSplitArrayLines[l].replace(sepSymbols, " ");
        QStringList inputStringSplitArray = inputStringSplitArrayLines[l].simplified().split(QRegularExpression("\\s+"), Qt::SplitBehaviorFlags::SkipEmptyParts); // rozdzielamy traktująac spacje jako separator

        if (parsSettings.timestampMode == serial::SysTimeStamp && inputStringSplitArray.last().contains("RDTSTAMP")){
            long tmpms = inputStringSplitArray.last().replace("RDTSTAMP", "").toInt();
            QTime tmpTime = QTime::fromMSecsSinceStartOfDay(tmpms);
            if(tmpTime.isValid())
            {
                latestTimeStamp_ms = tmpms;
            }
            else
            {
                latestTimeStamp_ms += parsSettings.fixinterval; //ToDo find out whats best when timestamp tag not present..
                if(latestTimeStamp_ms > MS_PER_DAY){latestTimeStamp_ms -= MS_PER_DAY;}//ToDo micros
            }
            inputStringSplitArray.removeLast();
        }
        for (auto i = 0; i < inputStringSplitArray.count(); ++i)
        {
            // Find external time...
            if (tstampmode == serial::ExternalTStamp)  //ToDo time format as intervals...
            {
                if (externalClockLabel.isEmpty() == false && inputStringSplitArray[i] == externalClockLabel
                    && (QTime::fromMSecsSinceStartOfDay((inputStringSplitArray[i + 1]).toInt()).isValid()))
                {
                    latestTimeStamp_ms = inputStringSplitArray[i + 1].toInt();
                }
                else if (externalClockLabel.isEmpty() == true)
                {
                    foreach (auto timeFormat, searchTimeFormatList)
                    {
                        if (QTime::fromString(inputStringSplitArray[i], timeFormat).isValid())
                        {
                            latestTimeStamp_ms = this->timeTomsSinceBeginOfDay(QTime::fromString(inputStringSplitArray[i], timeFormat));
                            break;
                        }
                    }

                    if (minimumTime != QTime(0, 0, 0) && maximumTime != QTime(0, 0, 0))
                    {
                        if (QTime::fromMSecsSinceStartOfDay(latestTimeStamp_ms) < minimumTime
                            || QTime::fromMSecsSinceStartOfDay(latestTimeStamp_ms) > maximumTime)
                        {
                            continue;
                        }
                    }
                }
            }

            // Labels +
            if (i == 0 && mainSymbols.match(inputStringSplitArray[0]).hasMatch())
            {
                listNumericData.append(inputStringSplitArray[i].toDouble());
                stringListLabels.append("Graph 0");
            }
            else if (i > 0 && mainSymbols.match(inputStringSplitArray[i]).hasMatch() && mainSymbols.match(inputStringSplitArray[i - 1]).hasMatch())
            {
                listNumericData.append(inputStringSplitArray[i].toDouble());
                stringListLabels.append("Graph " + QString::number(i));
            }
            else if (i > 0 && mainSymbols.match(inputStringSplitArray[i]).hasMatch() && !mainSymbols.match(inputStringSplitArray[i - 1]).hasMatch())
            {
                listNumericData.append(inputStringSplitArray[i].toDouble());
                stringListLabels.append(inputStringSplitArray[i - 1]);
            }
            else
            {
                continue; // We didnt find or add any new data points so lets not log time and skip to the next element on the list...
            }

            if (tstampmode == serial::ExternalTStamp)
            {
                listTimeStamp.append(latestTimeStamp_ms);
            }
            else if (tstampmode == serial::SysTimeStamp)
            {
               listTimeStamp.append(latestTimeStamp_ms); // old listTimeStamp.append(parserClock.currentTime().msecsSinceStartOfDay());
            }
            else if (tstampmode == serial::FixIntervalTStamp)
            {
                latestTimeStamp_ms += parsSettings.fixinterval;
                if(latestTimeStamp_ms > MS_PER_DAY){latestTimeStamp_ms -= MS_PER_DAY;}//ToDo micros
                listTimeStamp.append(latestTimeStamp_ms);
            }
            else //tstampmode == serial::NoTStamp
            {
                listTimeStamp.append(parserTimer.elapsed());
            }
        }
    }
}

void Parser::parseCSV(QString inputString, bool useExternalLabel, QString externalClockLabel)
{
    listNumericData.clear();
    stringListLabels.clear();
    listTimeStamp.clear();
    lineCount = 0;

    QStringList inputStringSplitArrayLines = inputString.split(QRegularExpression("[\\n+\\r+]"), Qt::SplitBehaviorFlags::SkipEmptyParts);
    lineCount = inputStringSplitArrayLines.count();

    QStringList csvLabels; // !

    for (auto l = 0; l < lineCount; ++l)
    {
        parsingProgressPercent = (float)l / lineCount * 100.0F;
        if (l % 50 == 0 && canReportProgress)
        {
            emit updateProgress(&parsingProgressPercent);
            QApplication::processEvents(); // Prevents app for freeezeng during processing large files and allows to udpate progress percent. A very cheap trick...
        }

        if (abortFlag)
        {
            abortFlag = false;
            break;
        }

        QRegularExpression mainSymbols("^[-+]?[0-9]*\\.?[0-9]+$");//(QRegularExpression::anchoredPattern("[+-]?\\d*\\.?\\d+")); // float only   //  QRegExp mainSymbols("[-+]?[0-9]*\.?[0-9]+");
        QRegularExpression alphanumericSymbols("\\w+");
        QRegularExpression sepSymbols("[=,]");

        inputStringSplitArrayLines[l].replace(sepSymbols, " ");
        inputStringSplitArrayLines[l].remove("\"");

        QStringList inputStringSplitArray = inputStringSplitArrayLines[l].simplified().split(QRegularExpression("\\s+"), Qt::SplitBehaviorFlags::SkipEmptyParts); // rozdzielamy traktująac spacje jako separator

        // Look for labels
        if (l == 0)
        {
            for (auto i = 0; i < inputStringSplitArray.count(); ++i)
            {
                if (!sepSymbols.match(inputStringSplitArray[i]).hasMatch())
                {
                    if (!csvLabels.contains(inputStringSplitArray[i]))
                        csvLabels.append(inputStringSplitArray[i]);
                }
            }
        }

        // Look for time reference
        for (auto i = 0; i < inputStringSplitArray.count(); ++i)
        {
            if (useExternalLabel == true && externalClockLabel.isEmpty() == false && i == csvLabels.indexOf(externalClockLabel))
            {
                qDebug() << "inputStringSplitArray: " + QString::number(inputStringSplitArray[i].toFloat());

                latestTimeStamp_ms = inputStringSplitArray[i].toLong();
                qDebug() << "TIME: " + QString::number(inputStringSplitArray[i].toFloat());
                qDebug() << "latestTimeStamp: " << latestTimeStamp_ms;
                break;
            }
            else if (useExternalLabel == false)
            {
                foreach (auto timeFormat, searchTimeFormatList)
                {
                    if (QTime::fromString(inputStringSplitArray[i], timeFormat).isValid())
                    {
                        latestTimeStamp_ms = this->timeTomsSinceBeginOfDay(QTime::fromString(inputStringSplitArray[i], timeFormat));

                        if (minimumTime != QTime(0, 0, 0) && maximumTime != QTime(0, 0, 0))
                        {
                            if (QTime::fromMSecsSinceStartOfDay(latestTimeStamp_ms) < minimumTime
                                || QTime::fromMSecsSinceStartOfDay(latestTimeStamp_ms) > maximumTime)
                            {
                                continue;
                            }
                        }

                        break;
                    }
                }
            }
        }

        // Look for data
        for (auto i = 0; i < inputStringSplitArray.count(); ++i)
        {
            if (mainSymbols.match(inputStringSplitArray[i]).hasMatch())
            {
                if (i >= csvLabels.count())
                    continue; // TODO ERROR REPORTING
                stringListLabels.append(csvLabels[i]);
                listNumericData.append(inputStringSplitArray[i].toDouble());
                listTimeStamp.append(latestTimeStamp_ms);
            }
        }
    }
}

void Parser::getCSVReadyData(QStringList *columnNames, QList<QList<double>> *dataColumns)
{
    QStringList labelStorage = this->getLabelStorage();
    QStringList tempColumnNames = labelStorage;
    tempColumnNames.removeDuplicates();
    QList<QList<double>> tempColumnsData;
    QList<double> numericDataList = this->getDataStorage();

    for (auto i = 0; i < tempColumnNames.count(); ++i)
    {
        tempColumnsData.append(*new QList<double>);

        while (labelStorage.contains(tempColumnNames[i]))
        {
            tempColumnsData[tempColumnsData.count() - 1].append(numericDataList.takeAt(labelStorage.indexOf(tempColumnNames[i])));
            labelStorage.removeAt(labelStorage.indexOf(tempColumnNames[i]));
        }
    }

    *columnNames = tempColumnNames;
    *dataColumns = tempColumnsData;
}

long Parser::timeTomsSinceBeginOfDay(QTime time)
{
    return MS_PER_DAY - long(time.msecsTo(QTime(0,0,0,0)));
}

void Parser::setParseSettings(int tstampMode, QString extClockLabel, int fixintv, double tbase_s, int prefltr)
{
    if(tstampMode > -1 && tstampMode <= 3)
    {
        parsSettings.timestampMode = static_cast<serial::SERIAL_TSTAMP_MODE>(tstampMode);
    }
    if(extClockLabel != "")
    {
        parsSettings.externalClockLabel = extClockLabel;
    }
    if(fixintv > 0)
    {
        parsSettings.fixinterval = fixintv;
    }
    if(tbase_s > 0)
    {
        parsSettings.timebase_s = tbase_s;
    }
    if(prefltr > -1 && prefltr <= 3)
    {
        parsSettings.prefilterMode = static_cast<PARSER_PRE_FILTER_MODE>(prefltr);
    }
}


QList<double> Parser::getDataStorage() { return dataStorage; }
QList<double> Parser::getListNumericValues() { return listNumericData; }
QList<long> Parser::getListTimeStamp() { return listTimeStamp; }
QList<long> Parser::getTimeStorage() { return timeStampStorage; }
QStringList Parser::getLabelStorage() { return labelStorage; }
QStringList Parser::getStringListLabels() { return stringListLabels; }
QStringList Parser::getStringListNumericData() { return stringListNumericData; }
QStringList Parser::getTextList() { return textStorage; }
void Parser::clearExternalClock() { latestTimeStamp_ms = 0;} //.setHMS(0, 0, 0, 0);

void Parser::restartChartTimer()
{
    parserTimer.restart();

}

void Parser::parserClockAddMSecs(int ms)
{
    parserClock = parserClock.addMSecs(ms);
    // parserTimer->start();
}

void Parser::appendSetToMemory(QStringList newlabelList, QList<double> newDataList, QList<long> newTimeList, QString text)
{
    labelStorage.append(newlabelList);
    dataStorage.append(newDataList);
    timeStampStorage.append(newTimeList);

    if (!text.isEmpty())
        textStorage.append(text);
}

void Parser::clearStorage()
{
    labelStorage.clear();
    dataStorage.clear();
    timeStampStorage.clear();
    textStorage.clear();
}

void Parser::clear()
{
    stringListLabels.clear();
    stringListNumericData.clear();
    listTimeStamp.clear();
}

void Parser::setParsingTimeRange(QTime minTime, QTime maxTime)  //ToDo to analyze
{
    minimumTime = minTime;
    maximumTime = maxTime;
}

void Parser::resetTimeRange()
{
    minimumTime.setHMS(0, 0, 0);
    maximumTime.setHMS(0, 0, 0);
}

void Parser::abort() { abortFlag = true; }
void Parser::setReportProgress(bool isEnabled) { canReportProgress = isEnabled; }

}//end namespace
