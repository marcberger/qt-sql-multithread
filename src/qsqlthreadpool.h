#ifndef QSQLTHREADPOOL_H
#define QSQLTHREADPOOL_H

#include <QThreadPool>
#include <QSqlError>
#include <QSqlQuery>
#include <QMutex>
#include <QPair>

#include "qsqltask.h"
#include "qcustomqsqlquery.h"



/**
  * @brief This class is a singleton
  *        It is designed to be called only by QCustomQsqlQuery
  *        It "hides" the fact that database is accessed through specific threads
  *        It contains a pool of threads, each thread having its own database connection
  */
class QCustomQsqlQuery;
class QSQLThreadPool : public  QObject
{
    Q_OBJECT

public:

    enum POOL_ERROR
    {
        NONE,
        CLEARED_FROM_QUEUE
    };

    static QString poolErrorAsString(POOL_ERROR error)
    {
        QString ret="NONE";
        switch(error)
        {
        case NONE:
            ret="NONE";
            break;

        case CLEARED_FROM_QUEUE:
            ret="The query has been droped because the waiting list had to be flushed. It has not been executed";
            break;
        default:
            ret="Unknown Error";
        }
        return ret;

    }

    // functions used to create and destroy the singleton
    static QSQLThreadPool *getInstance ();
    static void kill ();

signals:
public slots:
    /**
      * @brief a slot used to ask to run a SQL query.
      *         This slot identifies the caller with a call to sender()
      *         The query will be run on an independant connection thread.
      *         For the same caller, query will continue to be performed in the same connection thread unless the sender emits endOfTransaction signal
      *         Queries may be queued if no connection thread is available
      * @param query            The query to be run
      */
    bool runSQLQuery( const QString & query);

private slots:
    /**
      * @brief a slot used to release a connection thread when the QCustomQsqlQuery does not need it anymore
      *        The QCustomQsqlQuery is identified by a call to sender()
      */
    void canReleasethread();

private:
    //Number of connections established to the database
    static int s_database_NbOfSqlConnection;
    //Max number of sql queries waiting for a connection to the database
    static int s_database_QueryMaxQueueSize;
    static QMutex * s_mutex_singleton;

    static QSQLThreadPool * _singleton;
    explicit QSQLThreadPool(QObject *parent = 0);
    virtual ~QSQLThreadPool();
    //The list of connections to the sqldatabase
    class Privateconnection;
    class WaitingListElement;
    QList< Privateconnection > m_taskList;
    QList< WaitingListElement> m_waitingQueryList;
    static QString loggerId;
    //Thread in which the QSqlThreadPool object lives
    static QThread m_thread;

    //get the index of the relation "sender"/task in the m_taskList
    int getCallerIndex(QCustomQsqlQuery* psender);
    //Launch a query in a new thread
    void startQueryInNewThread(QCustomQsqlQuery* psender,QString query);
    //Put query in the waiting list
    void enqueueElement(WaitingListElement elementToEnqueue);

    /**
      * @brief A class used to describe a  query when no connection is available and it needs to be queued
      */
    class WaitingListElement
    {
        friend class QSQLThreadPool;
        WaitingListElement(QCustomQsqlQuery* i_psender, QString i_query) :
            psender(i_psender),
            query(i_query)
        {}

    public:
        QCustomQsqlQuery* psender;
        QString query;
    };

    /**
      * @brief A class used to describe a connection to the database
      * This class is designed to be used inside QSQLThreadPool
      * Each connection includes a a specific thread (pthread), a worker obeject(ptask)
      * and a reference to the QCustomQsqlQuery used to make the call (psender)
      */
    class Privateconnection
    {
        friend class QSQLThreadPool;
    public:
        Privateconnection(QThread* i_pthread,QSQLTask* i_ptask,QCustomQsqlQuery* i_psender) :
            pthread(i_pthread),
            ptask(i_ptask),
            psender(i_psender)
        {}

        virtual ~Privateconnection()
        {}

        Privateconnection(Privateconnection const& con)
        {
            pthread=con.pthread;
            ptask=con.ptask;
            psender=con.psender;
        }

    protected:
        QThread* pthread;
        QSQLTask* ptask;
        QCustomQsqlQuery* psender;
    private:


    };

};

#endif // QSQLTHREADPOOL_H
