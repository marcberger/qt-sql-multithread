#include "workerexample.h"
#include "src/qcustomqsqlquery.h"
#include "src/LoggerDef.h"

QString WorkerExample::loggerId=QLatin1String(LOGGER_SQL);

WorkerExample::WorkerExample(QString query,int workFrequencyMs,QObject *parent) :
    QObject(parent),
    m_query(query)
{
    m_internalTimer.setInterval(workFrequencyMs);
    QObject::connect(&m_internalTimer,SIGNAL(timeout()),this,SLOT(work()));
    m_internalTimer.start();;
}

void WorkerExample::work()
{
    QCustomQsqlQuery customquery(loggerId);
    bool res = customquery.exec(m_query);
    if(res==true)
    {
        if(customquery.lastError().type()==QSqlError::NoError)
        {
            QList<QVariant> result;
            while(customquery.next(result))
            {
                LOG_INFO(loggerId,QString("NEXT : %1")
                         .arg(result.at(0).toString()));
            }
            LOG_INFO(loggerId,QString("FINISHED Browsing results"));
        }
        else
        {
            LOG_ERROR(loggerId,QString("Error SQL query=[%1] -- err_type=[%2] -- err_numer=[%3] -- err_text=[%4]")
                      .arg(m_query)
                      .arg(qsql_error_as_string(customquery.lastError().type()))
                      .arg(customquery.lastError().number())
                      .arg(customquery.lastError().databaseText()));        }
    }
    else
    {
        LOG_ERROR(loggerId,QString("Error while trying to run the query=[%1] -- err_type=[%2] -- err_numer=[%3] -- err_text=[%4]")
                  .arg(m_query)
                  .arg(qsql_error_as_string(customquery.lastError().type()))
                  .arg(customquery.lastError().number())
                  .arg(customquery.lastError().databaseText()));
    }
}
