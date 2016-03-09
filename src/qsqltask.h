#ifndef QSQLTASK_H
#define QSQLTASK_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
//#include <QRunnable>
#include <QObject>
#include <QVariant>

class QSqlError;

/**
  * A class used to access a database connection
  * This class is a worker object designed to live in a dedicated thread (use the moveToThread command)
  */
class QSQLTask : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Create a QSQLTask Object. Instanciate a connection to a Database
     * @param isInitOK      a pointor indicating if the connection to the database has been successfull
     * @param id            The id of the connection (used to name the connection)
     * @param parent
     */
    explicit QSQLTask(bool * isInitOK, int id, QObject *parent = 0);
    /**
      @brief destructor. Remove dabasebase connection and closes it
      */
    virtual ~QSQLTask();

    /**
      * Set an sql query to be executed by this class when executeQuery is called
      */
    void setQuery(QString query);

signals:
    /**
      * @brief a signal indicating the the executeQuery slot call is finished
      * @brief a Slot to receive the results of the sqlQuery and of next() calls
      * @param isOk               Indicates if the last query has been run successfully or not
      * @param hasMore            Indicates if there are more results to get (more than the ones of presult)
      * @param presult            A first set of results for the query
      * @param lastError          The result of the last query
      * @param idTransaction      The Id of the transaction/thread
      */
    void finished(bool isOk, bool hasMore, QList< QList <QVariant> >* presult, QSqlError lastError, int idTask);

public slots:
    /**
      * @brief a slot used to run the query set by the setQuery function
      * @emits a finished signal when it is done
      */
    virtual void executeQuery();

    /**
      * @brief a slot used to browse more results
      * @emits a finished signal when it is done
      */
    virtual void next();
    /**
      @brief A slot called to free the last run query
      */
    virtual void finish();

private:

    //destroy the connection
    virtual void destroy();
    /** @brief Read results from m_last_query_run using next and load them inside result up to a max of max_lines_to_return
      *     set hasMore to true if there are more results to be browsed from the database for this query
      */
    void loadResults(int max_lines_to_return, bool &hasMore, QList< QList <QVariant> >& result);
    //the id of the qsqltask used to identify the database connection
    QString m_idAsString;
    //The id of the qsqltask
    int m_id;

    //The query to run when executeQuery is called
    QString m_query;
    //The QSqlQuery object used to interact with a query
    QSqlQuery m_last_query_run;

    bool m_isLastQueryRunOk;
    static QString loggerId;
    //Max number of results send by the finished signal
    static int s_max_buffered_results;
    //nb of columns of the last query run successfully
    int m_nbOfcolumns;
    //The driver to be used to access the database
    static QString s_databaseDriver;

    //Function to initialize database connection settings
    static bool initConnection(QString idDBToInitialize);

};

#endif // QSQLTASK_H
