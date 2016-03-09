#include <QtCore/QCoreApplication>
#include <QStringList>
#include <QDebug>
#include "src/qsqlthreadpool.h"
#include "src/qcustomqsqlquery.h"
#include "workerexample.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);


    /* Variables the CFG db connection*/
    if(QCoreApplication::instance()->argc()!=6)
    {
        qDebug()<<"Error with the parameters. expected <host> <dbname> <user> <pwd> <port> ";
        exit(-1);
    }


    QCustomQsqlQuery queryInitalizeBase;
    QStringList listOfTableNames;
    listOfTableNames.append("A");
    listOfTableNames.append("B");
    listOfTableNames.append("C");
    //    listOfTableNames.append("D");
    //    listOfTableNames.append("E");
    //    listOfTableNames.append("F");

    foreach (QString tableName, listOfTableNames) {
        QString dropResquest = QString("DROP TABLE %1;")
                .arg(tableName);
        QString createResquest = QString("CREATE TABLE %1 (COLUMN1 varchar(10));")
                .arg(tableName);

        queryInitalizeBase.exec(dropResquest);
        queryInitalizeBase.exec(createResquest);
        for(int j=0; j<1000; j++)
        {
            QString insertRequest = QString("INSERT INTO %1 (COLUMN1) VALUES (\'test%2%3\');")
                    .arg(tableName)
                    .arg(tableName)
                    .arg(j);
            queryInitalizeBase.exec(insertRequest);
        }
        queryInitalizeBase.exec("commit;");
    }

    sleep(10);
    QThread t1;
    WorkerExample* worker1=new WorkerExample("SELECT * FROM  A;",10000);
    worker1->moveToThread(&t1);

    QThread t2;
    WorkerExample* worker2=new WorkerExample("SELECT * FROM  B;",5000);
    worker2->moveToThread(&t2);

    QThread t3;
    WorkerExample* worker3=new WorkerExample("SELECT * FROM  C;",100);
    worker3->moveToThread(&t3);


    //    QThread t4;
    //    WorkerExample* worker4=new WorkerExample("SELECT * FROM  D;",100);
    //    worker4->moveToThread(&t4);

    //    QThread t5;
    //    WorkerExample* worker5=new WorkerExample("SELECT * FROM  E;",10);
    //    worker5->moveToThread(&t5);

    //    QThread t6;
    //    WorkerExample* worker6=new WorkerExample("SELECT * FROM  F;",10);
    //    worker6->moveToThread(&t6);

    t1.start();
    t2.start();
    t3.start();
    //    t4.start();
    //    t5.start();
    //    t6.start();

    return a.exec();
    QSQLThreadPool::kill();

}
