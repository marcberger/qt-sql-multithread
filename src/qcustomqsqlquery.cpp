#include "qcustomqsqlquery.h"
#include "LoggerDef.h"
#include "qsqlthreadpool.h"
#include <QString>
#include <QEventLoop>

QCustomQsqlQuery::QCustomQsqlQuery(QString loggerId_p) :
    QObject(NULL),
    loggerId(loggerId_p),
    m_result(NULL),
    m_bAreConnectDone(false),
    m_bHasMore(false)
{
    LOG_TRACE_BEGIN(loggerId,QString(""));
    LOG_TRACE_END(loggerId,QString(""));
}

QCustomQsqlQuery::~QCustomQsqlQuery()
{
    emit endOfTransaction();
    delete m_result;
    m_result=NULL;
}

bool QCustomQsqlQuery::exec(const QString& requete)
{
    bool ret=false;
    LOG_TRACE_BEGIN(loggerId,QString("requete=[%1]")
                    .arg(requete));

    //Clean memory just in case it has not been done yet
    delete m_result;
    m_result=NULL;
    m_bHasMore=false;
    //Get the QSQLThreadPool singleton
    QSQLThreadPool * pool = QSQLThreadPool::getInstance();

    //Check if the connect between the execSQLQuery signal and the runSQLQuery slot of the QSQLThreadPool singleton has been done
    if(!m_bAreConnectDone)
    {
        QObject::connect(this,SIGNAL(execSQLQuery(QString)),pool,SLOT(runSQLQuery(QString)));
        m_bAreConnectDone=true;
    }

    //Create an eventLoop to wait for the end of the query execution
    //This loop will be broken  when we receive a queryResultReceived signal
    QEventLoop wait;
    QObject::connect(this,SIGNAL(queryResultReceived()),&wait,SLOT(quit()));

    //Emit a execSQLQuery signal. It is connected to the runSQLQuery slot of the QSQLThreadPool singleton
    emit execSQLQuery(requete);

    //Launch the eventloop to wait for the result
    wait.exec();

    //disconnect the connection to the wait eventLoop (maybe not necessary as this Object will be deleted)
    QObject::disconnect(this,SIGNAL(queryResultReceived()),&wait,SLOT(quit()));

    //Compute the result of the execution
    if(m_execStatus==true)
    {
        ret=m_execStatus;
    }
    else
    {
        ret=(m_lastError.type()==QSqlError::NoError);
    }

    LOG_TRACE_END(loggerId,QString("m_execStatus=%1 - ret=%2")
                  .arg(m_execStatus)
                  .arg(ret));
    return ret;
}


void QCustomQsqlQuery::handleSQLQueryFinished(bool isOk, bool hasMore, QList< QList <QVariant> >* presult,QSqlError lastError, int idTransaction)
{
    LOG_TRACE_BEGIN(loggerId,QString("m_idTransaction=%1").arg(idTransaction));
    //delete any existing local value
    delete m_result;

    //    store results
    m_execStatus=isOk;
    m_lastError=lastError;
    m_result=new QList< QList <QVariant> >(*presult);
    m_bHasMore=hasMore;
    //Delete object pointor created in the connection thread
    delete presult;
    presult=NULL;

    //Emits a queryResultReceived signal that will break the waiting loop inside the exec function
    emit queryResultReceived();
    LOG_TRACE_END(loggerId,QString("m_idTransaction=%1").arg(idTransaction));
}


void QCustomQsqlQuery::handleSQLQueryError(int errorCode)
{
    LOG_TRACE_BEGIN(loggerId,QString("error=%1")
                  .arg(QSQLThreadPool::poolErrorAsString((QSQLThreadPool::POOL_ERROR)errorCode)));

    //Clean memory and store the error as the last known error
    m_execStatus=false;
    QSqlError sqlError("",QSQLThreadPool::poolErrorAsString((QSQLThreadPool::POOL_ERROR)errorCode),QSqlError::ConnectionError);
    m_lastError=sqlError;
    delete m_result;
    m_result=NULL;
    m_bHasMore=false;

    //Emits a queryResultReceived signal that will break the waiting loop inside the exec function
    emit queryResultReceived();

    LOG_TRACE_END(loggerId,QString(""));
}


bool QCustomQsqlQuery::next( QList <QVariant> &value)
{
//    LOG_TRACE_BEGIN(loggerId,QString("m_bHasMore=%1 - m_result.size=%2")
//                  .arg(m_bHasMore)
//                  .arg(m_result->size()));
    value.clear();
    bool ret=true;

    //We have some buffered results ==> Take one result from the local buffer
    if(!m_result->isEmpty())
    {
        value=m_result->takeFirst();
        ret=true;
    }
    //No buffered value and we known that more results are availables
    //==> make a call to the database connection to get more results
    else if(m_bHasMore)
    {
        //An event loop to wait for the end of the query
        QEventLoop wait;
        QObject::connect(this,SIGNAL(queryResultReceived()),&wait,SLOT(quit()));

        ret=false;
        m_bHasMore=false;
        emit getSQLQueryNext();
        wait.exec();
        QObject::disconnect(this,SIGNAL(queryResultReceived()),&wait,SLOT(quit()));
        if(!m_result->isEmpty())
        {
            value=m_result->takeFirst();
            ret=true;
        }
        else
        {
            ret=false;
        }
    }
    //No more data ==> return false
    else
    {
        ret=false;
    }
//    LOG_TRACE_END(loggerId,QString("ret=%1").arg(ret));

    return ret;
}
void QCustomQsqlQuery::clear()
{
    emit endOfTransaction();
    delete m_result;
    m_result=NULL;
}
