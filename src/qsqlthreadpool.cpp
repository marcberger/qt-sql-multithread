#include "qsqlthreadpool.h"
#include <QtCore>
#include <QSqlError>

#include "qcustomqsqlquery.h"
#include "LoggerDef.h"


QString QSQLThreadPool::loggerId=QLatin1String(LOGGER_SQL);
QSQLThreadPool * QSQLThreadPool::_singleton=NULL;
QThread QSQLThreadPool::m_thread;
QMutex *QSQLThreadPool::s_mutex_singleton=new QMutex();

//Number of connections established to the database
int QSQLThreadPool::s_database_NbOfSqlConnection=2;
//Max number of sql queries waiting for a connection to the database
int QSQLThreadPool::s_database_QueryMaxQueueSize=2;

QSQLThreadPool::QSQLThreadPool(QObject *parent) :
    QObject(parent)
{
    // Critical: register new type so that this signal can be
    // dispatched across thread boundaries by Qt using the event
    // system
    qRegisterMetaType< QSqlError >( "QSqlError" );

    //Create the connection threads and workers objects
    for (int i=0;i<QSQLThreadPool::s_database_NbOfSqlConnection;i++)
    {
        bool isInitOk;
        QThread * newThread = new QThread();
        QSQLTask * newTask = new QSQLTask(&isInitOk,i);
        if(!isInitOk)
        {
            LOG_FATAL(loggerId,QString("QSQLThreadPool::QSQLThreadPool: Error while Initializing a connection thread"));
            exit(-1);
        }
        else
        {
            //Add the infos of this connection in the list of connections
            m_taskList.append(Privateconnection(newThread,newTask,NULL));
            //start the thread dedicated to the connection
            newThread->start();
            //Put the worker in the dedicated thread
            newTask->moveToThread(newThread);
        }
    }
}

QSQLThreadPool *QSQLThreadPool::getInstance ()
{
    QMutexLocker lock(s_mutex_singleton);
    if (NULL == _singleton)
    {
        _singleton =  new QSQLThreadPool();
        _singleton->moveToThread(&m_thread);
        m_thread.start();
    }
    else
    {
        //TODO : log
        //std::cout << "singleton already created!" << std::endl;
    }

    return _singleton;
}

void QSQLThreadPool::kill ()
{
    QMutexLocker lock(s_mutex_singleton);
    if (NULL != _singleton)
    {
        m_thread.quit();
        delete _singleton;
        _singleton = NULL;
    }
}

QSQLThreadPool::~QSQLThreadPool()
{
    qDebug()<<"~QSQLThreadPool:1";
    while(!m_taskList.isEmpty())
    {
        qDebug()<<"~QSQLThreadPool:1a";
        Privateconnection element=m_taskList.takeFirst();
        if(element.pthread)
        {
            element.pthread->quit();
            element.pthread->wait(10*1000);
        }
        delete element.pthread;
        element.pthread=NULL;
        delete element.ptask;
        element.ptask=NULL;
        element.psender=NULL;
        qDebug()<<"~QSQLThreadPool:1b";
    }
    qDebug()<<"~QSQLThreadPool:2";
}

bool QSQLThreadPool::runSQLQuery( const QString & query)
{
    bool ret=false;
    QCustomQsqlQuery* psender = (QCustomQsqlQuery *) sender();
    QString s;
    QTextStream stream(&s);
    stream<<psender;
    LOG_TRACE_BEGIN(loggerId,QString("psender=%1 - Query=%2").arg(s).arg(query));

    //We want to reuse a transaction
    int index=getCallerIndex(psender);

    //The the caller has already a connection associated to it ==> Reuse it to run the query
    if(index>=0)
    {
        Privateconnection element=m_taskList.at(index);
        LOG_DEBUG(loggerId,QString("REUSE index=%1")
                  .arg(index));
        element.ptask->setQuery(query);
        QMetaObject::invokeMethod(element.ptask, "executeQuery",Qt::AutoConnection);
        ret=true;
    }
    //No connection associated with this caller ==> We need to associate a thread with it an then run the query
    //First we need to check if the waiting list of queries is empty or not
    //List is empty
    else if(m_waitingQueryList.isEmpty())
    {
        LOG_DEBUG(loggerId,QString("psender=%1 - m_waitingQueryList is empty ==> Launch the startQueryInNewThread").arg(s));
        startQueryInNewThread(psender,query);

    }
    //List is not empty ==> Enqueue this query
    else
    {
        LOG_DEBUG(loggerId,QString("psender=%1 - m_waitingQueryList is not empty ==> queue this query").arg(s));

        WaitingListElement element(psender,query);
        enqueueElement(element);
    }
    LOG_TRACE_END(loggerId,QString("psender=%1").arg(s));
    return ret;
}

void QSQLThreadPool::enqueueElement(WaitingListElement elementToEnqueue)
{
    //If the list is full ==> We will drop the full list
    //Callers will be notified by a call to their handleSQLQueryError slot
    if(m_waitingQueryList.length()>=QSQLThreadPool::s_database_QueryMaxQueueSize)
    {
        LOG_WARN(loggerId,QString("The query waiting list is full ==> It will be dropped and all queued queries will be lost"));


        while(!m_waitingQueryList.isEmpty())
        {
            WaitingListElement element = m_waitingQueryList.takeFirst();
            QString s;
            QTextStream stream(&s);
            stream<<element.psender;
            LOG_DEBUG(loggerId,QString("Discard query : [%1] of psender [%2]")
                      .arg(element.query)
                      .arg(s));
            //invoke the correct slot
            QMetaObject::invokeMethod(element.psender, "handleSQLQueryError", Qt::AutoConnection,
                                      Q_ARG(int, CLEARED_FROM_QUEUE));
        }

        //Clear the list
        m_waitingQueryList.clear();
    }

    //OK so now we are sure there is some space in the waiting list so we enqueue this query
    m_waitingQueryList.append(elementToEnqueue);
    QString s;
    QTextStream stream(&s);
    stream<<elementToEnqueue.psender;
    LOG_DEBUG(loggerId,QString("Enqueue Query [%1] of psender [%2] - m_waitingQueryList.length=%3")
              .arg(elementToEnqueue.query)
              .arg(s)
              .arg(m_waitingQueryList.length()));
}

void QSQLThreadPool::startQueryInNewThread(QCustomQsqlQuery* psender,QString query)
{
    LOG_TRACE_BEGIN(loggerId,QString(""));
    int index=0;
    bool do_cont=true;

    //Find a free thread, associate it with this query&sender and then call executeQuery on the QSqlTask item
    while(index<m_taskList.size() &&  do_cont==true)
    {
        LOG_TRACE(loggerId,QString("index=%1 -- do_cont=%2")
                  .arg(index)
                  .arg(do_cont));
        Privateconnection element=m_taskList.at(index);
        if(element.psender==NULL)
        {
            QString s;
            QTextStream stream(&s);
            stream<<psender;
            LOG_TRACE(loggerId,QString("Insert psender=%1 at index=%2 -- query=%3")
                      .arg(s)
                      .arg(index)
                      .arg(query));
            element.psender=psender;
            element.ptask->setQuery(query);
            m_taskList.replace(index,element);
            QObject::connect(element.ptask,SIGNAL(finished(bool,bool,QList<QList<QVariant> >*,QSqlError,int)),psender,SLOT(handleSQLQueryFinished(bool,bool,QList<QList<QVariant> >*,QSqlError,int)));
            //BLOCKING
            QObject::connect(psender,SIGNAL(endOfTransaction()),this,SLOT(canReleasethread()),Qt::BlockingQueuedConnection);
            QObject::connect(psender,SIGNAL(getSQLQueryNext()),element.ptask,SLOT(next()));

            QMetaObject::invokeMethod(element.ptask, "executeQuery",Qt::AutoConnection);

            do_cont=false;
            LOG_TRACE(loggerId,QString("index=%1 -- do_cont=%2")
                      .arg(index)
                      .arg(do_cont));
        }
        index++;
    }

    if(do_cont)
    {
        QString s;
        QTextStream stream(&s);
        stream<<psender;
        LOG_DEBUG(loggerId,QString("No Task available ==> queue the query of psender=%1").arg(s));
        WaitingListElement element(psender,query);
        enqueueElement(element);
    }
    LOG_TRACE_END(loggerId,QString(""));
}

void QSQLThreadPool::canReleasethread()
{
    QCustomQsqlQuery* psender = (QCustomQsqlQuery *) sender();
    QString s;
    QTextStream stream(&s);
    stream<<psender;
    LOG_TRACE_BEGIN(loggerId,QString("disconnect -- psender=%1").arg(s));
    bool found=false;
    int index=getCallerIndex(psender);
    //Release the task associated with the sender()
    //Disconnect all the connections between the sender and the task
    //Re-put this task in the m_taskList
    //Then if the waiting list is not empty ==> Launch the next query of the list (associate the sender...)
    if(index>=0)
    {
        Privateconnection element = m_taskList.at(index);
        LOG_TRACE(loggerId,QString("disconnect the task at index %1").arg(index));

        found=true;
        QMetaObject::invokeMethod(element.ptask, "finish",Qt::AutoConnection);
        //disconnect all the signals
        element.ptask->disconnect();
        element.psender=NULL;
        m_taskList.replace(index,element);
        //Re-use the thead if needed
        if(!m_waitingQueryList.isEmpty())
        {
            WaitingListElement waitingElement=m_waitingQueryList.takeFirst();
            LOG_DEBUG(loggerId,QString("Take Query from Queue (m_waitingQueryList.length=%1)")
                      .arg(m_waitingQueryList.length()));
            element.psender=waitingElement.psender;
            startQueryInNewThread(waitingElement.psender,waitingElement.query);
        }
    }
    else
    {
        LOG_ERROR(loggerId,QString("The psender [%1] is not present in the list").arg(s));
    }
    LOG_TRACE_END(loggerId,QString("disconnect -- psender=%1").arg(s));
}

int QSQLThreadPool::getCallerIndex(QCustomQsqlQuery* psender)
{
    int ret=-1;
    int index=0;
    while(ret==-1 && index<m_taskList.length())
    {
        Privateconnection element = m_taskList.at(index);
        if(element.psender==psender)
        {
            ret=index;
            break;
        }
        index++;
    }
    return ret;
}
