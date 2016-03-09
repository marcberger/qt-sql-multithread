#include "qsqltask.h"
#include "LoggerDef.h"
#include <QSqlQuery>
#include <QCoreApplication>

int QSQLTask::s_max_buffered_results=100;
QString QSQLTask::s_databaseDriver="QMYSQL";
QString QSQLTask::loggerId = QLatin1String(LOGGER_SQL);

QSQLTask::QSQLTask(bool *initOk,int id, QObject *parent) :
    QObject(parent),
    m_id(id),
    m_query(" "),
    m_isLastQueryRunOk(false),
    m_nbOfcolumns(0)
{
    m_idAsString=QString("%1").arg(m_id);
    qDebug()<<m_idAsString;

    *initOk=true;
    //Add the database connection
    QSqlDatabase::addDatabase(QSQLTask::s_databaseDriver,m_idAsString);

    //Initialize the connection infos like
    //HostName,Port,DatabaseName,setUserName,Password...
    if(!QSQLTask::initConnection(m_idAsString))
    {
        LOG_FATAL_END(loggerId,QString("Failed to init the Connection for m_idAsString=[%1]").arg(m_idAsString));
        QSqlDatabase::removeDatabase(m_idAsString);
        *initOk=false;
    }
    else
    {
        //Ok Connection has been correctly initialized ==> So we open it
        QSqlDatabase::database(m_idAsString,false).open();
        if (QSqlDatabase::database(m_idAsString,false).isOpenError())
        {
            //If we didn't succeed to connect ==> Its a major issue
            QString error_msg = QString("err_type=[%1] -- err_numer=[%2] -- err_text=[%3]")
                    .arg(qsql_error_as_string(QSqlDatabase::database(m_idAsString,false).lastError().type()))
                    .arg(QSqlDatabase::database(m_idAsString,false).lastError().number())
                    .arg(QSqlDatabase::database(m_idAsString,false).lastError().databaseText());
            LOG_FATAL_END(loggerId,QString("Failed to open MySql Connection for m_idAsString=[%1] -- ERROR=%2")
                          .arg(m_idAsString)
                          .arg(error_msg));
            QSqlDatabase::removeDatabase(m_idAsString);
            *initOk=false;
        }
        else
        {
            LOG_INFO(loggerId,QString("Connection Opened for m_idAsString=[%1]").arg(m_idAsString));
        }
    }
    LOG_TRACE_END(loggerId,QString("m_idAsString=%1 - initOk=%2")
                  .arg(m_idAsString)
                  .arg(*initOk));
}

void QSQLTask::setQuery(QString query)
{
    m_query=query;
}

QSQLTask::~QSQLTask()
{
    qDebug()<<"~QSQLTask:BEGIN";
    LOG_TRACE_BEGIN(loggerId,QString(""));
    destroy();
    LOG_TRACE_END(loggerId,QString(""));
    qDebug()<<"~QSQLTask:END";
}

void QSQLTask::finish()
{
    LOG_TRACE_BEGIN(loggerId,QString(""));
    m_last_query_run.clear();
    m_nbOfcolumns=0;
    LOG_TRACE_END(loggerId,QString(""));
}


void QSQLTask::destroy()
{
    LOG_TRACE_BEGIN(loggerId,QString(""));
    qDebug()<<"~destroy:1b";
    QSqlDatabase::database(m_idAsString,false).close();
    qDebug()<<"~destroy:2";
    QSqlDatabase::removeDatabase(m_idAsString);
    LOG_TRACE_END(loggerId,QString(""));
    qDebug()<<"~destroy:3";

}
void QSQLTask::executeQuery()
{


    LOG_DEBUG_BEGIN(loggerId,QString("m_idAsString=%1 - query=%2")
                    .arg(m_idAsString)
                    .arg(m_query));

    //Reinit various variable to start with a "clean" state
    m_isLastQueryRunOk=false;
    m_last_query_run = QSqlQuery(QSqlDatabase::database(m_idAsString,false));
    m_nbOfcolumns=0;

    //Execute the query
    if(m_last_query_run.exec(m_query)==true)
    {
        QSQL_TRACE_LOG(loggerId,m_last_query_run,m_query,QString("m_idAsString=%1").arg(m_idAsString));
        m_isLastQueryRunOk=true;
    }
    else
    {
        //Query failed
        //Handle the specific case of a connection lost ==>Try to reconnect and try to re-exec
        if(m_last_query_run.lastError().type()==QSqlError::StatementError && m_last_query_run. lastError().number()==2006)
        {
            QSQL_ERROR_LOG(loggerId,m_last_query_run,m_query,QString("m_idAsString=%1 ==> Reconnect").arg(m_idAsString));
            QSqlDatabase::database(m_idAsString,true);
            if (!QSqlDatabase::database(m_idAsString,false).isOpen())
            {
                LOG_ERROR(loggerId,QString("m_idAsString=%1 - Error QSqlDatabase::database(m_idAsString).open : %2")
                          .arg(m_idAsString)
                          .arg(QSqlDatabase::database(m_idAsString,false).lastError().text()));
                m_isLastQueryRunOk=false;
            }
            else
            {
                LOG_TRACE(loggerId,QString("m_idAsString=%1 - Reconnected successfully ==> relaunch the query").arg(m_idAsString));
                m_isLastQueryRunOk=m_last_query_run.exec(m_query);
            }
        }
        else
        {
            QSQL_ERROR_LOG(loggerId,m_last_query_run,m_query,QString("m_idAsString=%1").arg(m_idAsString));
        }
    }
    LOG_TRACE(loggerId,QString("AFTER REQUEST -- size=%1").arg(m_last_query_run.size()));

    QList< QList <QVariant> >* presult = new QList< QList <QVariant> >();
    bool hasMore=false;

    if(m_isLastQueryRunOk==true)
    {
        loadResults(s_max_buffered_results,hasMore,(*presult));
    }

    LOG_DEBUG_END(loggerId,QString("m_idAsString=%1 - query=%2 -resultSize=%3 --hasMore=%4")
                  .arg(m_idAsString)
                  .arg(m_query)
                  .arg(presult->length())
                  .arg(hasMore));
    emit finished(m_isLastQueryRunOk, hasMore, presult,m_last_query_run.lastError(),m_id);
}

void QSQLTask::loadResults(int max_lines_to_return, bool &hasMore, QList< QList <QVariant> >& result)
{
    result.clear();
    hasMore=false;


    if(m_last_query_run.next())
    {
        if(m_nbOfcolumns==0)
        {
            //Count the number of columns of the result
            //This code is pretty bad as it only iterate over the columns untill we get an invalid column
            //The problem is that this attempt to access an invalid columns displays a qWarning message
            //like QMYSQLResult::data: column 1 out of range
            int index=0;
            bool cont = true;
            while(cont==true)
            {
                if(!m_last_query_run.value(index).isValid())
                {
                    cont=false;
                }
                else
                {
                    index++;
                }
            }
            //we try to access element at index 2 ==> it is not valid so we have elements 0 and 1 available
            //so a total of 2 columns ==>index==nb_of_columns
            m_nbOfcolumns=index;
        }
        if(m_nbOfcolumns>0)
        {
            int nb_lines=0;
            do
            {
                nb_lines++;
                QList <QVariant> newLine;
                for(int index=0; index<m_nbOfcolumns; index++)
                {
                    QVariant newColumn = m_last_query_run.value(index);
                    newLine.append(newColumn);
                }
                result.append(newLine);
            }
            while(nb_lines<max_lines_to_return && m_last_query_run.next());

            if(nb_lines==max_lines_to_return && m_last_query_run.isValid())
            {
                hasMore=true;
            }
        }
    }
}



void QSQLTask::next()
{
    LOG_TRACE_BEGIN(loggerId,QString(""));
    QList< QList <QVariant> >* presult = new QList< QList <QVariant> >();
    bool hasMore=false;

    if(m_isLastQueryRunOk==true)
    {
        loadResults(s_max_buffered_results,hasMore,(*presult));
    }

    LOG_DEBUG_END(loggerId,QString("m_idAsString=%1 - query=%2 -resultSize=%3")
                  .arg(m_idAsString)
                  .arg(m_query)
                  .arg(presult->length()));
    emit finished(m_isLastQueryRunOk, hasMore, presult,m_last_query_run.lastError(),m_id);
    LOG_TRACE_END(loggerId,QString(""));
}


bool QSQLTask::initConnection(QString idDBToInitialize)
{

    /* Variables the CFG db connection*/
    if(QCoreApplication::instance()->argc()>=6)
    {
        QString sParamDbCfgHost= QCoreApplication::instance()->argv()[1];
        QString sParamDbCfgName = QCoreApplication::instance()->argv()[2];
        QString sParamDbCfgUser = QCoreApplication::instance()->argv()[3];
        QString sParamDbCfgPasswd = QCoreApplication::instance()->argv()[4];
        int iParamDbCfgPort = atoi(QCoreApplication::instance()->argv()[5]);

        QSqlDatabase db_to_initialize=QSqlDatabase::database(idDBToInitialize,false);
        db_to_initialize.setHostName(sParamDbCfgHost);
        db_to_initialize.setPort(iParamDbCfgPort);
        db_to_initialize.setDatabaseName(sParamDbCfgName);
        db_to_initialize.setUserName(sParamDbCfgUser);
        db_to_initialize.setPassword(sParamDbCfgPasswd);
    }
    else
    {
        LOG_FATAL(loggerId,QString("Error with the parameters. expected <host> <dbname> <user> <pwd> <port>"));
        return false;
    }


    return true;
}
