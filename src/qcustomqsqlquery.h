#ifndef QCUSTOMQSQLQUERY_H
#define QCUSTOMQSQLQUERY_H

#include <QSqlQuery>
#include <QObject>
#include <QSqlError>
#include <QVariant>
#include "PoolError.h"

class QSqlError;
/**
  @brief This class is a designed to handle sql connections the same way QSqlQuery does.
  * Its specificity is to access the connection to the database through QSQLThreadPool
  * So database action is performed in a thread specific to the connection
  * This class does not yet provide all the QSqlQuery services... It should still be improved
  */
class QCustomQsqlQuery :
        public QObject
{
    Q_OBJECT
public:
    /**
      * @brief creates a new QCustomQsqlQuer
      * @param loggerId_p   Identifier of the logger
      */
    QCustomQsqlQuery(QString loggerId_p="");

    /**
      *  @brief destructor. Free local memory, end the transaction
     */
    virtual ~QCustomQsqlQuery();

    /**
      * @brief execute the <query> sql query.
      *        Other calls of this function on the same object will be run in the same transaction unless endOfTransaction signal is emited
      *        This function blocks on an EvenLoop waiting for a queryResultReceived signal
      * @param  query: The sql query to be run
      * @returns : true if ok
      */
    bool exec(const QString& query);

    /**
      * @brief browse the next result returned by the last query run through the exec function
      *        This function may block on an EvenLoop waiting for a queryResultReceived signal
      *         Note: This function browse the set of results we got after the exec query or after the last call to next
      *               If there are more data available calls to the database to get them through the dedicated connection will be performed
      * @param  value : A list of QVariant to store the Next Raw content (one value per column)
      * @returns true if a raw was available (you can loop over results with a while(next..()) )
      */
    bool next( QList <QVariant> &value);

    /**
      * @brief Free local memory, end the transaction. After this, any call to exec will create a new transaction
      * @emits a endOfTransaction signal
      */
    void clear();
    /**
      * @brief to the the last Error
      * @returns the last error
      */
    QSqlError lastError() {return m_lastError;}

public slots:
    /**
      * @brief a Slot to receive the results of the sqlQuery and of next() calls
      * @emits a queryResultReceived signal
      * @param isOk               Indicates if the last query has been run successfully or not
      * @param hasMore            Indicates if there are more results to get (more than the ones of presult)
      * @param presult            A first set of results for the query
      * @param lastError          The result of the last query
      * @param idTransaction      The Id of the transaction. Should be used for further calls on the same transaction or to relase it
      */
    void handleSQLQueryFinished(bool isOk, bool hasMore, QList< QList <QVariant> >* presult,QSqlError lastError, int idTransaction);
    /**
      * @brief a Slot used to receive notification that an error happened while launching the sqlQuery (it is not an sql error)
      * @emits a queryResultReceived signal
      * @param error    An indentifier for the error (see PoolError)
      */
    void handleSQLQueryError(int error);

private:
signals:
    /**
      * @brief signal to tell the receiver to execute an sql query
      * @param  The query to be executed
      */
    void execSQLQuery(QString);
    /**
      * @brief signal to tell the reveiver to end the transaction.
      *        we will be notified that the query is finished through the handleSQLQueryFinished slot
      */
    void endOfTransaction();
    /**
      * @brief signal to get another set of results for the last run query.
      *         We will receive the next set of results through handleSQLQueryFinished
      */
    void getSQLQueryNext();
    /**
      * @brief an internal signal used to signal that results of a query or of a next call have been received
      */
    void queryResultReceived();
protected:
private:
    QString loggerId;

    /**
      An internal list to store the results of a query
      There is one entry for each raw
      Each raw is a list of columns represented as QVariant
      WARN : This pointer should be freed correctly
    */
    QList< QList <QVariant> >* m_result;
    /**
      Bool indicatinf the status ok or not) of the last executed query
      */
    bool m_execStatus;
    /**
      Store the result of the last executed query
      */
    QSqlError m_lastError;
    /**
      Memorize if the connect (QObject::) operations have been done to link this class and the QSqlThreadPool
      */
    bool m_bAreConnectDone;
    /**
      A flag indicating if the last run query has more results which should be collected using a getSQLQueryNext signal
      */
    bool m_bHasMore;


};

#endif // QCUSTOMQSQLQUERY_H
