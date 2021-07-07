#ifndef DBMNG_H
#define DBMNG_H

#include <QObject>
#include <QtSql>
#include <QSqlQuery>
#include <QQueue>
#include "syslogger.h"
#include "logdlg.h"

class dbmng : public QThread
{
    Q_OBJECT

    struct querydata
    {
        int seq;
        int channel;
        QString querystring;
        QByteArray vehicleimg;
        QByteArray vehicleplateimg;
    };

    enum DatabaseState
    {
        OPEN = 0x0000,
        CLOSE = 0x0001
    };

public:
    explicit dbmng(int loglevel = LOG_INFO,QThread *parent = 0);
    ~dbmng();
    void SetLogLevel(int loglevel);
    void run();
    void stop();
    bool connect();
    void disconnect() ;
    void init();
    bool checkcreatedb();
    bool createdb();
    // Trigger
    bool createdata(int channel,int index);
    // after recognition
    bool updatedata(int channel,int index);
    bool updatekindofvehicle(int channel,int index);
    bool updatetransflag(int channel,int index,QDateTime shottime,int sendflag);
    bool checkNcreatetable();
    bool createtable(QString strtablename);
    bool checkdeletetable();
    bool deletetable(QString strdbtablename);
    bool dropdb(QString strdbname);

    //OBU Match table
    struct OBUData
    {
        QByteArray obunum;
        QByteArray obuvehicle;
    };

    bool SetOBUDB(bool updateflag,int id, QString sControlNo, QString fileName);
    bool OBU_UPDATE();
    bool checkOBUdb(QString strdbname);
    bool createOBUdb(QString strdbname);
    bool setOBUtable();
    bool checkOBUtable(QString strdbname,QString strtablename);
    bool createOBUtable(QString strdbname,QString strtablename);
    bool insertOBUdata(QString strdbname,QString strtablename, QList<OBUData> obulist);
    QByteArray searchOBUdata(QByteArray obunum);


signals:
    void OBUUpdateResult(int id,QString sControlNo,bool bupdate, QString sFilename);
public slots:

public:
    QSqlDatabase m_db;
    QString dbip;
    int dbport;
    QString dbname;
    QString user;
    QString password;
    QString m_datecheck;
    bool brun;
    QQueue<querydata> wquery;
    Syslogger *log;
    LogDlg *plogdlg0;
    LogDlg *plogdlg1;
    QMutex wmutex;
    QMutex sqlmutex;
    #define MAX_QUERY_COUNT  100
    int m_state;
    //1분동안 접속이 안되면 db restart
    #define DB_RESTART_COUNT 120
    int m_dbrestartcount;

    int m_loglevel;

    bool bOBUupdate;
    QString OBUdbname;
    QString OBUtablename;
    QString OBUMatchtable;
    QFile *OBUfile;
    QString OBUfileName;
    int m_OBUfilecount;
    int m_OBUfileNum;
    int m_OBUfdataindex;
    QString OBUUpdateNo;
    int m_OBUcenterid;


};

#endif // DBMNG_H
