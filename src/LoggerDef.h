#ifndef LOGGERDEF_H_
#define LOGGERDEF_H_
#include <stdio.h>
#include <QFileInfo>
#include <QThread>
#include <QSqlError>
#include <QDebug>
#include <QTime>

#define LOGGER_SQL "LOGGER_SQL"

//cf. http://www.decompile.com/cpp/tips/error_reporting.htm
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
//#define AT TOSTRING(__PRETTY_FUNCTION__) ":"  TOSTRING(__LINE__)


// Macros
#define FILE_BASENAME QFileInfo(__FILE__).fileName()

//To Get the thread ID because QThread::currentThreadId returns a QT::HANDLE specific for each platform
#if defined(Q_WS_MAC)
    #define THREAD_ID ((const char *)(unsigned char*)(void*) QThread::currentThreadId())
#elif defined(Q_WS_WIN)
    #define THREAD_ID ((const char *)(unsigned char*)(void*) QThread::currentThreadId())
#elif defined(Q_WS_X11)
    #define THREAD_ID QThread::currentThreadId()
#elif defined(Q_WS_QWS)
    #define THREAD_ID (int) ((const char *)(unsigned char*)(void*) QThread::currentThreadId())
#elif defined(Q_OS_SYMBIAN)
    #define THREAD_ID QThread::currentThreadId() // equivalent to TUint32
#endif


#ifndef  DISPLAY_THREAD_IN_LOGS

#define LOCATE_TEMPLATE "%1::%2:%3:"
#define LOCATE_TEMPLATE_BEGIN "<<%1::%2:%3:"
#define LOCATE_TEMPLATE_END ">>%1::%2:%3:"
#define LOCATE QString(LOCATE_TEMPLATE).arg(FILE_BASENAME).arg(__FUNCTION__).arg(__LINE__).toStdString().c_str()
#define LOCATE_BEGIN QString(LOCATE_TEMPLATE_BEGIN).arg(FILE_BASENAME).arg(__FUNCTION__).arg(__LINE__).toStdString().c_str()
#define LOCATE_END QString(LOCATE_TEMPLATE_END).arg(FILE_BASENAME).arg(__FUNCTION__).arg(__LINE__).toStdString().c_str()

#else

#define LOCATE_TEMPLATE "%1::%2:%3[threadId=%4]:"
#define LOCATE_TEMPLATE_BEGIN "<<%1::%2:%3[threadId=%4]:"
#define LOCATE_TEMPLATE_END ">>%1::%2:%3[threadId=%4]:"
#define LOCATE QString(LOCATE_TEMPLATE).arg(FILE_BASENAME).arg(__FUNCTION__).arg(__LINE__).arg(THREAD_ID).toStdString().c_str()
#define LOCATE_BEGIN QString(LOCATE_TEMPLATE_BEGIN).arg(FILE_BASENAME).arg(__FUNCTION__).arg(__LINE__).arg(THREAD_ID).toStdString().c_str()
#define LOCATE_END QString(LOCATE_TEMPLATE_END).arg(FILE_BASENAME).arg(__FUNCTION__).arg(__LINE__).arg(THREAD_ID).toStdString().c_str()
#endif


#define TWO_PARAM_TEMPLATE "%1 %2"




#define LOG_TRACE(LOGGER,MSG) qDebug()<<QTime::currentTime().toString("hh:mm:ss.zzz")<<" "<<QString(TWO_PARAM_TEMPLATE).arg(LOCATE).arg(MSG)
#define LOG_TRACE_BEGIN(LOGGER,MSG) qDebug()<<QTime::currentTime().toString("hh:mm:ss.zzz")<<" "<<QString(TWO_PARAM_TEMPLATE).arg(LOCATE_BEGIN).arg(MSG)
#define LOG_TRACE_END(LOGGER,MSG) qDebug()<<QTime::currentTime().toString("hh:mm:ss.zzz")<<" "<<QString(TWO_PARAM_TEMPLATE).arg(LOCATE_END).arg(MSG)


#define LOG_DEBUG(LOGGER,MSG) qDebug()<<QTime::currentTime().toString("hh:mm:ss.zzz")<<" "<<QString(TWO_PARAM_TEMPLATE).arg(LOCATE).arg(MSG)
#define LOG_DEBUG_BEGIN(LOGGER,MSG) qDebug()<<QTime::currentTime().toString("hh:mm:ss.zzz")<<" "<<QString(TWO_PARAM_TEMPLATE).arg(LOCATE_BEGIN).arg(MSG)
#define LOG_DEBUG_END(LOGGER,MSG) qDebug()<<QTime::currentTime().toString("hh:mm:ss.zzz")<<" "<<QString(TWO_PARAM_TEMPLATE).arg(LOCATE_END).arg(MSG)


#define LOG_INFO(LOGGER,MSG) qDebug()<<QTime::currentTime().toString("hh:mm:ss.zzz")<<" "<<QString(TWO_PARAM_TEMPLATE).arg(LOCATE).arg(MSG)
#define LOG_INFO_BEGIN(LOGGER,MSG) qDebug()<<QTime::currentTime().toString("hh:mm:ss.zzz")<<" "<<QString(TWO_PARAM_TEMPLATE).arg(LOCATE_BEGIN).arg(MSG)
#define LOG_INFO_END(LOGGER,MSG) qDebug()<<QTime::currentTime().toString("hh:mm:ss.zzz")<<" "<<QString(TWO_PARAM_TEMPLATE).arg(LOCATE_END).arg(MSG)

#define LOG_WARN(LOGGER,MSG) qDebug()<<QTime::currentTime().toString("hh:mm:ss.zzz")<<" "<<QString(TWO_PARAM_TEMPLATE).arg(LOCATE).arg(MSG)
#define LOG_WARN_BEGIN(LOGGER,MSG) qDebug()<<QTime::currentTime().toString("hh:mm:ss.zzz")<<" "<<QString(TWO_PARAM_TEMPLATE).arg(LOCATE_BEGIN).arg(MSG)
#define LOG_WARN_END(LOGGER,MSG) qDebug()<<QTime::currentTime().toString("hh:mm:ss.zzz")<<" "<<QString(TWO_PARAM_TEMPLATE).arg(LOCATE_END).arg(MSG)


#define LOG_ERROR(LOGGER,MSG) qDebug()<<QTime::currentTime().toString("hh:mm:ss.zzz")<<" "<<QString(TWO_PARAM_TEMPLATE).arg(LOCATE).arg(MSG)
#define LOG_ERROR_BEGIN(LOGGER,MSG) qDebug()<<QTime::currentTime().toString("hh:mm:ss.zzz")<<" "<<QString(TWO_PARAM_TEMPLATE).arg(LOCATE_BEGIN).arg(MSG)
#define LOG_ERROR_END(LOGGER,MSG) qDebug()<<QTime::currentTime().toString("hh:mm:ss.zzz")<<" "<<QString(TWO_PARAM_TEMPLATE).arg(LOCATE_END).arg(MSG)

#define LOG_FATAL(LOGGER,MSG) qDebug()<<QTime::currentTime().toString("hh:mm:ss.zzz")<<" "<<QString(TWO_PARAM_TEMPLATE).arg(LOCATE).arg(MSG)
#define LOG_FATAL_BEGIN(LOGGER,MSG) qDebug()<<QTime::currentTime().toString("hh:mm:ss.zzz")<<" "<<QString(TWO_PARAM_TEMPLATE).arg(LOCATE_BEGIN).arg(MSG)
#define LOG_FATAL_END(LOGGER,MSG) qDebug()<<QTime::currentTime().toString("hh:mm:ss.zzz")<<" "<<QString(TWO_PARAM_TEMPLATE).arg(LOCATE_END).arg(MSG)


/* LOGS for a QSQL query */

inline QString qsql_error_as_string(QSqlError::ErrorType error)
{
    switch(error)
    {
    case QSqlError::NoError:
        return "NoError";

    case QSqlError::ConnectionError:
        return "ConnectionError";

    case QSqlError::StatementError:
        return "StatementError";

    case QSqlError::TransactionError:
        return "TransactionError";

    case QSqlError::UnknownError:
        return "UnknownError";

    default:
        return "UNKNOWN_ERROR";
    }
}

/* params :
  query : QSqlQuery
  query_string : a type compatible with QString::arg
  msg : a type compatible with QString::arg
  */
#define QSQL_LOG_MSG_TRACE(query,query_string,msg) \
    ((query.isSelect())? \
    QString("SQL query=[%1] -- size=[%2] -- err_type=[%3] -- err_numer=[%4] -- err_text=[%5] %6").arg(query_string).arg(query.size()).arg(qsql_error_as_string(query.lastError().type())).arg(query.lastError().number()).arg(query.lastError().databaseText()).arg(msg) \
    : QString("SQL query=[%1] -- numRowsAffected=[%2] -- err_type=[%3] -- err_numer=[%4] -- err_text=[%5] %6").arg(query_string).arg(query.numRowsAffected()).arg(qsql_error_as_string(query.lastError().type())).arg(query.lastError().number()).arg(query.lastError().databaseText()).arg(msg) \
    )

#define QSQL_TRACE_LOG(LOGGER,query,query_string,msg)  LOG_TRACE(LOGGER,QSQL_LOG_MSG_TRACE(query,query_string,msg))

#define QSQL_LOG_MSG_ERROR(query,query_string,msg)  QString("SQL ERROR %1").arg(QSQL_LOG_MSG_TRACE(query,query_string,msg))

#define QSQL_ERROR_LOG(LOGGER,query,query_string,msg) qDebug()<<QTime::currentTime().toString("hh:mm:ss.zzz")<<" "<<QSQL_LOG_MSG_ERROR(query,query_string,msg);
#endif /* LOGERDEF_H_ */
