#ifndef WORKEREXAMPLE_H
#define WORKEREXAMPLE_H

#include <QObject>
#include <QTimer>
class WorkerExample : public QObject
{
    Q_OBJECT
public:
    explicit WorkerExample(QString query="", int workFrequencyMs=1000,QObject *parent = 0);
    void setQuery(QString query) { m_query=query;}
signals:

public slots:
    void work();

private:
    QString m_query;
    QTimer m_internalTimer;

    static QString loggerId;
};

#endif // WORKEREXAMPLE_H
