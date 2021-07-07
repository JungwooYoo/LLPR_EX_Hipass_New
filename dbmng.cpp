#include "dbmng.h"
#include "commonvalues.h"
#include <QApplication>

dbmng::dbmng(int loglevel,QThread *parent) :
    QThread(parent)
{
    m_datecheck = QDateTime::currentDateTime().toString("dd");
    brun = false;
    m_state = CLOSE;
    log = new Syslogger(this,"dbmng",true,loglevel);
    m_loglevel = loglevel;

    m_dbrestartcount=0;

    //OBU
    bOBUupdate = false;
    OBUdbname = "OBUdb";
    OBUtablename = "OBUDATA";
    OBUMatchtable = "";
    OBUfile = NULL;
    OBUfileName = "";
    m_OBUfilecount = 1;
    m_OBUfileNum = 1;
}

dbmng::~dbmng()
{
    log->deleteLater();
}

void dbmng::SetLogLevel(int loglevel)
{
    log->m_loglevel = loglevel;
    m_loglevel = loglevel;

}
void dbmng::stop()
{
    brun = false;
}
void dbmng::run()
{
    brun = true;
    int seccount=0;
    bool brtnquery;
    QDateTime pretime;
    QDateTime aftertime;
    int difftime;


    QString logstr = QString("dbmng start");
    log->write(logstr,LOG_ERR); qDebug() << logstr;

    while(brun)
    {
        sqlmutex.lock();
        try
        {

            //2016/10/19
            //date table create & connection check
            if(!checkNcreatetable())
            {
                disconnect();
            }
            // DB connection check
            if( m_db.isOpen())
            {
                m_dbrestartcount = 0;

                if(!wquery.isEmpty())
                {
                    wmutex.lock();
                    querydata data = wquery.dequeue();
                    wmutex.unlock();

                    int channel = data.channel;

                    LogDlg *plogdlg;
                    if(channel == 1) plogdlg = plogdlg1;
                    else plogdlg = plogdlg0;

                    QSqlQuery query(m_db);
                    QString querystring;

                    querystring = data.querystring;

                    if( query.prepare(querystring))
                    {
                        if(data.vehicleimg.size() > 0 )
                        {
                           query.bindValue(":carimagedata",data.vehicleimg);
                           if(data.vehicleplateimg.size() > 0)
                           {
                               query.bindValue(":carplateimagedata",data.vehicleplateimg);
                           }

                        }
                        pretime = QDateTime::currentDateTime();
                        brtnquery = query.exec();
                        aftertime = QDateTime::currentDateTime();
                        difftime = pretime.msecsTo(aftertime);


                        if( querystring.size() > 300 )
                        {
                            logstr = QString("DB EXEC(%1)[%2|%3] : %4")
                                    .arg(data.seq).arg(brtnquery ? "Success" : "Fail").arg(difftime).arg(QString("update vehicle image"));

                            //qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz]") <<  logstr;
                            if(plogdlg != NULL) { plogdlg->logappend(LogDlg::logrecog,logstr);} //insert
                            log->write(logstr,LOG_NOTICE);
                        }
                        else
                        {
                            logstr = QString("DB EXEC(%1)[%2|%3]")
                                    .arg(data.seq).arg(brtnquery ? "Success" : "Fail").arg(difftime);

                            //qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz]") <<  logstr << " : " << querystring;
                            if(plogdlg != NULL) { plogdlg->logappend(LogDlg::logcamera,logstr);}
                            log->write(logstr,LOG_NOTICE);
                        }


                    }
                    else
                    {
                        logstr = QString("DB Prepare FAIL(%1)")
                                .arg(data.seq);
                        qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz]") <<  logstr << " : " << (querystring.size() > 300 ?  QString("update vehicle image"):querystring);
                        if(plogdlg != NULL) { plogdlg->logappend(LogDlg::logrecog,logstr);}
                        log->write(logstr,LOG_NOTICE);
                        disconnect();


                    }

                }

                //OBU update
                if(bOBUupdate)
                {
                    OBU_UPDATE();
                    if(!bOBUupdate)
                    {
                        setOBUtable();
                    }
                }
                if(commonvalues::obuMatch > 0 && seccount%10 == 0) //5sec
                {
                    if(OBUMatchtable.isNull() || OBUMatchtable.isEmpty())
                    {
                        setOBUtable();
                    }
                }
            }
            else
            {
                m_dbrestartcount++;
                if(m_dbrestartcount > DB_RESTART_COUNT )
                {
                    QString strcmd = QString("/etc/init.d/mysql restart");
                    system(strcmd.toStdString().c_str());
                    m_dbrestartcount = 0;

                    logstr = QString("DB Restart : %1").arg(strcmd);
                    qDebug() << logstr;
                    log->write(logstr,LOG_ERR);

                }
                //2016/10/19
                logstr = QString("DB Connection Fail");
                qDebug() << logstr;
                log->write(logstr,LOG_NOTICE);
                connect();
            }

            //check database table  & remove data;
            if(seccount%7200 == 0 ) checkdeletetable();
        }
        catch( ... )
        {
            logstr = QString("--------- Error DB Thread -----------");
            log->write(logstr,LOG_ERR);            qDebug() << logstr;
        }
        sqlmutex.unlock();
        msleep(500);
        seccount++;
    }

    logstr = QString("dbmng end");
    log->write(logstr,LOG_ERR); qDebug() << logstr;
}

bool dbmng::connect()
{
    bool brtn = false;
    //mysql
    //QSqlDatabase qdb
    m_db = QSqlDatabase::addDatabase("QMYSQL");
    m_db.setHostName(dbip);
    m_db.setPort(dbport);
    m_db.setUserName(user);
    m_db.setPassword(password);
    if( m_db.open())
    {
//        m_db = qdb;
        brtn = true;
        m_state = OPEN;
        QString logstr = QString("Database Connection");
        log->write(logstr,LOG_NOTICE);
    }
    else m_state |= CLOSE;
    return brtn;
}

void dbmng::disconnect()
{
   m_db.close();
   m_state |= CLOSE;
   QString logstr = QString("Database Disconnection");
   log->write(logstr,LOG_NOTICE);
}

void dbmng::init()
{
    dbip = commonvalues::databaseinfo.db_ip;
    dbport = commonvalues::databaseinfo.db_port;
    dbname = commonvalues::databaseinfo.db_name;
    user = commonvalues::databaseinfo.db_user;
    password = commonvalues::databaseinfo.db_password;

    if(m_db.isOpen())
    {
        disconnect();
    }
    connect();

}

bool dbmng::checkcreatedb()
{
//    bool brtn = connect();
//    if(!brtn) return false;

    if(!m_db.isOpen())
    {
        if( !m_db.open())
        {
            return false;
        }
    }

    //check DB
    QSqlQuery query(m_db);
    QString querystring;
    querystring = "show databases like '" + dbname +"'";
    query.exec(querystring);
    if(query.next()) //  DB check;
    {
        // "database lprdb 있음"
        qDebug() << dbname <<"database is already";
    }
    else
    {
        if(!createdb())
        {
            disconnect();
            return false;
        }
    }

    if(!checkNcreatetable())
    {
        disconnect();
        return false;
    }
    //disconnect();
    return true;
}
bool dbmng::createdb()
{
    bool brtn = true;

    if(m_db.isOpen())
    {
        QSqlQuery query(m_db);
        QString querystring = "create database " + dbname;
        if(query.exec(querystring)){}
        else
        {
            // fail
            qDebug() << "create " + dbname + " database fail";
            brtn = false;
        }
    }
    else brtn = false;

    return brtn;
}

bool dbmng::updatedata(int channel, int index)
{

    bool brtn = true;

    QDateTime entrytime = QDateTime::fromString(QString((const char *)commonvalues::vehicledatalist[channel][index].car_entrytime),"yyyyMMdd-HHmmss");
    QString querystring;
    QString strtablename = QString("vehicleimage_%1").arg(entrytime.toString("yyyyMMdd"));
    QString qrecognum = QString((const char *)commonvalues::vehicledatalist[channel][index].recognum);
    int recogresult = commonvalues::vehicledatalist[channel][index].recogresult;

    querystring = QString("update %1.%2 ").arg(dbname).arg(strtablename);
    //PlateNumber varchar
    querystring += "set PlateNumber= '" + QString((const char *)commonvalues::vehicledatalist[channel][index].recognum) + "',";
    //CarImage longblob
    querystring += "CarImage = :carimagedata ,";
    //CarPlateImage lognblob
    //if(qrecognum != "xxxxxx")
    //sdw 2017/05/07
    if(recogresult > 0)
    {
        querystring += "CarPlateImage = :carplateimagedata ,";
    }
    //CarImageWidth int
    querystring += "CarImageWidth = " + QString::number(commonvalues::cameraSys[channel].cam_image_width) + ",";
    //CarImageHeight int
    querystring += "CarImageHeight = " + QString::number(commonvalues::cameraSys[channel].cam_image_height) + ",";
    //CarPlateImageWidth int
    querystring += "CarPlateImageWidth = " + QString::number(commonvalues::vehicledatalist[channel][index].plate_width) + ",";
    //CarPlateImageHeight int
    querystring += "CarPlateImageHeight = " + QString::number(commonvalues::vehicledatalist[channel][index].plate_height) + ",";
    //CarPlateImage SX int
    querystring += "CarPlateImageSX= " + QString::number(commonvalues::vehicledatalist[channel][index].plate_x) + ",";
    //CarPlateImage SY int
    querystring += "CarPlateImageSY = " + QString::number(commonvalues::vehicledatalist[channel][index].plate_y) + ",";
    //ReverseFlag int
    querystring += "ReverseFlag = " + QString::number(commonvalues::vehicledatalist[channel][index].direction) + ",";
    //RecongnitionWaitTime int
    querystring += "RecognitionWaitTime = " + QString::number((int)commonvalues::vehicledatalist[channel][index].recogtime)+ ",";
    //TransFlag  int
    querystring += "TransFlag = " + QString::number(commonvalues::vehicledatalist[channel][index].bVehicleNotification ? 1 : 0);
    //where
    //sdw //2016/09/22  insert index
    QString strindex = QString("%1-%2").arg(entrytime.toString("yyyyMMddHHmmss")).arg(commonvalues::vehicledatalist[channel][index].seq);
    querystring += QString(" where Index1 = '%1'").arg(strindex);
    //    querystring += QString(" where SeqNum = %1 and ShotTime = '%2'")
//            .arg(index).arg(commonvalues::vehicledatalist[channel][index].car_entrytime.toString("yyyy-MM-dd HH:mm:ss"));

    querydata data;
    data.channel = channel;
    data.seq = commonvalues::vehicledatalist[channel][index].seq;
    data.querystring.append(querystring);
    data.vehicleimg.append(QByteArray((const char*)commonvalues::vehicledatalist[channel][index].saveimg,commonvalues::vehicledatalist[channel][index].saveimglen));
    //if(qrecognum != "xxxxxx")
    if(recogresult > 0)
    {
        data.vehicleplateimg.append(QByteArray((const char*)commonvalues::vehicledatalist[channel][index].saveplateimg,commonvalues::vehicledatalist[channel][index].saveplateimglen));
    }
    if(wquery.size() <= MAX_QUERY_COUNT)
    {
        wmutex.lock();
        wquery.enqueue(data);
        wmutex.unlock();
    }
    else
    {
        qDebug() << QString("[%1]DB ENQUEUE Full(%2) : %5")
                        .arg(QDateTime::currentDateTime().toString("hh:mm:ss:zzz")).arg(data.seq)
                    .arg(querystring.size() > 300 ?  QString("update vehicle image"):querystring);
    }

    return brtn;
}

bool dbmng::updatekindofvehicle(int channel, int index)
{

    bool brtn = true;

    QDateTime entrytime = QDateTime::fromString(QString((const char *)commonvalues::vehicledatalist[channel][index].car_entrytime),"yyyyMMdd-HHmmss");
    QString querystring;
    QString strtablename = QString("vehicleimage_%1").arg(entrytime.toString("yyyyMMdd"));

    querystring = QString("update %1.%2 ").arg(dbname).arg(strtablename);
    //KindofVehicle int
    querystring += QString("set KindofVehicle = %1").arg(commonvalues::vehicledatalist[channel][index].size_type);

    //where
    //sdw //2016/09/22  insert index
    QString strindex = QString("%1-%2").arg(entrytime.toString("yyyyMMddHHmmss")).arg(commonvalues::vehicledatalist[channel][index].seq);
    querystring += QString(" where Index1 = '%1'").arg(strindex);
//    querystring += QString(" where SeqNum = %1 order by ShotTime desc limit 1;")
//            .arg(index);

    querydata data;
    data.channel = channel;
    data.seq = commonvalues::vehicledatalist[channel][index].seq;
    data.querystring.append(querystring);
    if(wquery.size() <= MAX_QUERY_COUNT)
    {
        wmutex.lock();
        wquery.enqueue(data);
        wmutex.unlock();
    }
    else
    {
        qDebug() << QString("[%1]DB ENQUEUE Full(%2) : %5")
                        .arg(QDateTime::currentDateTime().toString("hh:mm:ss:zzz")).arg(data.seq)
                    .arg(querystring.size() > 300 ?  QString("update vehicle image"):querystring);
    }

    return brtn;
}


bool dbmng::updatetransflag(int channel,int index,QDateTime shottime,int sendflag)
{

    bool brtn = true;

    QDateTime entrytime = QDateTime::fromString(QString((const char *)commonvalues::vehicledatalist[channel][index].car_entrytime),"yyyyMMdd-HHmmss");
    QString querystring;
    QString strtablename = QString("vehicleimage_%1").arg(entrytime.toString("yyyyMMdd"));

    querystring = QString("update %1.%2 ").arg(dbname).arg(strtablename);
    //KindofVehicle int
    querystring += QString("set TransFlag = %1").arg(sendflag);
    //where
    querystring += QString(" where SeqNum = %1 and ShotTime = '%2' order by ShotTime desc limit 1;")
            .arg(index).arg(shottime.toString("yyyy-MM-dd HH:mm:ss"));

    querydata data;
    data.channel = channel;
    data.seq = commonvalues::vehicledatalist[channel][index].seq;
    data.querystring.append(querystring);
    if(wquery.size() <= MAX_QUERY_COUNT)
    {
        wmutex.lock();
        wquery.enqueue(data);
        wmutex.unlock();
    }
    else
    {
        qDebug() << QString("[%1]DB ENQUEUE Full(%2) : %5")
                        .arg(QDateTime::currentDateTime().toString("hh:mm:ss:zzz")).arg(data.seq)
                    .arg(querystring.size() > 300 ?  QString("update vehicle image"):querystring);
    }

    return brtn;
}

bool dbmng::checkNcreatetable()
{
    bool brtn = true;

    if(m_db.isOpen())
    {
        QSqlQuery query(m_db);
        QString querystring;
        QString strtablename = QString("vehicleimage_%1").arg(QDateTime::currentDateTime().toString("yyyyMMdd"));

        //connection check;
        querystring = QString("select count(table_name) from information_schema.tables where table_name='%1' and table_schema = '%2'")
                .arg(strtablename).arg(dbname);


        if(query.exec(querystring))
        { //connection ok

            if( query.next())
            {
                int count = query.value(0).toInt();
                //qDebug() << QString("checkNcreatetable Success(count:%1|%2)").arg(count).arg(querystring);
                if( count == 0)
                {
                    if( !createtable(strtablename))
                    {
                        brtn = false;
                    }
                }
            }
            else brtn = false;
        }
        else
        { //disconnection
            qDebug() << QString("checkNcreatetable File(%1)").arg(querystring);
            brtn = false;
        }

    }
    else brtn = false;

    return brtn;
}

bool dbmng::createtable(QString strtablename)
{
    bool brtn = true;

    if(m_db.isOpen())
    {
        QSqlQuery query(m_db);
        QString querystring;

        querystring = " create table " + dbname +"." + strtablename + " (";
        //sdw //2016/09/22  insert index
        querystring += "	Index1 varchar(50) NULL, ";
        querystring += "	SeqNum int NOT NULL, ";
        querystring += "	TgNumber int NULL, ";
        querystring += "	PlateNumber varchar(50) NULL, ";
        querystring += "	ShotTime datetime NOT NULL, ";
        querystring += "	CarImage longblob NULL, ";
        querystring += "	CarPlateImage longblob NULL, ";
        querystring += "	CarImageWidth int NULL, ";
        querystring += "	CarImageHeight int NULL, ";
        querystring += "	CarPlateImageWidth int NULL, ";
        querystring += "	CarPlateImageHeight int NULL, ";
        querystring += "	CarPlateImageSX int NULL, ";
        querystring += "	CarPlateImageSY int NULL, ";
//        querystring += "	UseFlag int NULL, ";
        querystring += "	CDS int NULL, ";
        querystring += "	BR int NULL, ";
        querystring += "	Shutter int NULL, ";
        querystring += "	Gain int NULL, ";
        querystring += "	Speed int NULL, ";
//        querystring += "	ImageLength int NULL, ";
        querystring += "	Light int NULL, ";
        querystring += "	ReverseFlag int NULL, ";
//        querystring += "	ColorFlag int NULL, ";
//        querystring += "	RecognitionFlag int NULL, ";
//        querystring += "	RecognitionTime datetime NULL, ";
        querystring += "	RecognitionWaitTime int NULL, ";
        querystring += "	KindofVehicle int NULL, ";
        querystring += "    TransFlag int NULL,";
        //sdw //2016/09/22  insert index
        querystring += "    INDEX idx1 (Index1) )";

        if(query.exec(querystring))
        {
            //"lprdb database, vehicleimage table 생성 성공"
            qDebug() <<  QString("create table Success(%1.%2)").arg(dbname).arg(strtablename);
            brtn = true;
        }
        else
        {
            //"create vehicleimage 실패"
            qDebug() <<  QString("create table Fail(%1.%2)").arg(dbname).arg(strtablename);
            //disconnect();
            brtn = false;

        }
    }
    else brtn = false;

    return brtn;
}

bool dbmng::createdata(int channel, int index)
{

    bool brtn = true;

    QDateTime entrytime = QDateTime::fromString(QString((const char *)commonvalues::vehicledatalist[channel][index].car_entrytime),"yyyyMMdd-HHmmss");
    QString querystring;
    QString strtablename = QString("vehicleimage_%1").arg(entrytime.toString("yyyyMMdd"));

    querystring = QString("insert into %1.%2(Index1,SeqNum,TgNumber,ShotTime,CDS,BR,Shutter,Gain,Light,KindofVehicle) values( ").arg(dbname).arg(strtablename);

    //sdw //2016/09/22
    //Index varchar
    QString strindex = QString("%1-%2").arg(entrytime.toString("yyyyMMddHHmmss")).arg(commonvalues::vehicledatalist[channel][index].seq);
    querystring += "'"+ strindex +"',";
    //SeqNum int
    querystring += QString::number(commonvalues::vehicledatalist[channel][index].seq) + ",";
    //TgNumber int
    querystring += QString::number(commonvalues::vehicledatalist[channel][index].car_num) + ",";
    //ShotTime varchar
    querystring += "'" + entrytime.toString("yyyy-MM-dd HH:mm:ss")+ "',";
    //CDS int
    querystring += QString::number(commonvalues::currentvalue[channel].cur_cds) + ",";
    //BR int
    querystring += QString::number(commonvalues::currentvalue[channel].cur_br) + ",";
    //Shutter int
    querystring += QString::number(commonvalues::currentvalue[channel].cur_shutter) + ",";
    //Gain int
    querystring += QString::number(commonvalues::currentvalue[channel].cur_gain) + ",";
    //Light int
    querystring += QString::number(commonvalues::currentvalue[channel].cur_light) + ",";
    //차량 종류 대신에 채널 int
    querystring += QString::number(channel) + ")";

    querydata data;
    data.channel = channel;
    data.seq = commonvalues::vehicledatalist[channel][index].seq;
    data.querystring.append(querystring);
    if(wquery.size() <= MAX_QUERY_COUNT)
    {
        wmutex.lock();
        wquery.enqueue(data);
        wmutex.unlock();
    }
    else
    {
        qDebug() << QString("[%1]DB ENQUEUE Full(%2) : %5")
                        .arg(QDateTime::currentDateTime().toString("hh:mm:ss:zzz")).arg(data.seq)
                    .arg(querystring.size() > 300 ?  QString("update vehicle image"):querystring);
    }

    return brtn;
}

bool dbmng::checkdeletetable()
{
    bool brtn = true;

    //image save duration
    if(commonvalues::databaseinfo.db_storageDuration > 0)
    {
        if(m_db.isOpen())
        {
            QSqlQuery query(m_db);
            QDateTime standartday = QDateTime::currentDateTime().addDays( (qint64)(- commonvalues::databaseinfo.db_storageDuration));

            QString querystring = QString("select table_name from information_schema.tables where table_name like 'vehicleimage%' and create_time < '%1'")
                    .arg(standartday.toString("yyyy-MM-dd"));
            if(query.exec(querystring))
            {
               qDebug() << QString("check delete table Success(%1)").arg(querystring);
               if(query.next())
               {
                   QString strtablename = QString("%1.%2").arg(dbname).arg(query.value(0).toString());

                   while(query.next())
                   {
                       strtablename += QString(", %1.%2").arg(dbname).arg(query.value(0).toString());
                   }
                   if(!deletetable(strtablename)) brtn=false;
               }
            }
            else
            {
                qDebug() << QString("check delete table Fail(%1)").arg(querystring);
                brtn=false;
            }

        //select table_name from information_schema.tables where table_name like 'vehicleimage%' and create_time < '2016-10-14' limit 1

        }


    }
    return brtn;
}


bool dbmng::deletetable(QString strdbtablename)
{
    bool brtn = true;

    if(m_db.isOpen())
    {
        QSqlQuery query(m_db);

        QString querystring = QString("drop table %1").arg(strdbtablename);

        if( query.exec(querystring))
        {
            qDebug() << QString("deletetable Sucess(%1)").arg(querystring);
        }
        else
        {
            qDebug() << QString("deletetable Fail(%1)").arg(querystring);
            brtn = false;
        }

    }
    else brtn = false;

    return brtn;
}

bool dbmng::dropdb(QString strdbname)
{
    bool brtn = true;

    if(m_db.isOpen())
    {
        QSqlQuery query(m_db);

        QString querystring = QString("drop database %1").arg(strdbname);

        if( query.exec(querystring))
        {
            qDebug() << QString("drop database Sucess(%1)").arg(querystring);
        }
        else
        {
            qDebug() << QString("drop database Fail(%1)").arg(querystring);
            brtn = false;
        }

    }
    else brtn = false;

    return brtn;
}

bool dbmng::SetOBUDB(bool updateflag, int id, QString sControlNo, QString fileName)
{
    if(bOBUupdate) return false;

    m_OBUcenterid = id;
    OBUtablename = QString("OBUDATA_%1").arg(QDateTime::currentDateTime().toString("yyyyMMdd"));
    OBUfileName = QString(fileName.data(),fileName.size());
    m_OBUfilecount = 1;
    OBUUpdateNo = QString(sControlNo.data(),sControlNo.size());

    sqlmutex.lock();
    if(!checkOBUdb(OBUdbname))
    {
        if(!createOBUdb(OBUdbname))
        {
            sqlmutex.unlock();
            return false;
        }
    }

    if(checkOBUtable(OBUdbname,OBUtablename))
    {
        QString strtablename = QString("%1.%2").arg(OBUdbname).arg(OBUtablename);
        if(!deletetable(strtablename))
        {
            sqlmutex.unlock();
            return false;
        }
    }
    sqlmutex.unlock();

    bOBUupdate = updateflag;

    return true;
}

bool dbmng::OBU_UPDATE()
{
    bool brtn = false;
    QString logstr;
    QString Download_path =  QApplication::applicationDirPath() + "/Download";
    QString strobufilename = QString("%1/%2_%3").arg(Download_path).arg(OBUfileName).arg(m_OBUfilecount,3,10,QChar('0'));

    if(OBUfile == NULL)
    { //OBU File Open

        logstr = QString("OBU DB Update Start!!");
        if(plogdlg0 != NULL) { plogdlg0->logappend(LogDlg::logcenter,logstr);}
        log->write(logstr,LOG_NOTICE);

        OBUfile = new QFile(strobufilename);
        if(!OBUfile->open(QIODevice::ReadOnly))
        {
            bOBUupdate = false;
            logstr = QString("Can't Open OBU File(%1). Stop OBU DB Update!!").arg(strobufilename);
            if(plogdlg0 != NULL) { plogdlg0->logappend(LogDlg::logcenter,logstr);}
            log->write(logstr,LOG_NOTICE);

            QString strobudbtable = QString("%1.%2").arg(OBUdbname).arg(OBUtablename);
            deletetable(strobudbtable);

            emit OBUUpdateResult(m_OBUcenterid,OBUUpdateNo,false,OBUfileName);

            return brtn;
        }
        else
        {
            logstr = QString("----- Open OBU File(%1)!!").arg(strobufilename);
            if(plogdlg0 != NULL) { plogdlg0->logappend(LogDlg::logcenter,logstr);}
            log->write(logstr,LOG_NOTICE);
        }
        m_OBUfdataindex = 16;  // OBU header : 16byte( 영업소코드,당일날짜,파일번호,테이블개총개수,현재파일개수)
        QByteArray obuheader = OBUfile->read(m_OBUfdataindex);

        //BCD
        int index = 0;
        int ManagerID = obuheader.mid(index,2).toHex().toInt(); index += 2;
        int OBUDate = obuheader.mid(index,4).toHex().toInt(); index += 4;
        int FileNum = obuheader.mid(index,2).toHex().toInt(); index += 2;
        int OBUTotalCount = obuheader.mid(index,4).toHex().toInt(); index += 4;
        int OBUFileCount = obuheader.mid(index,4).toHex().toInt(); index += 4;

        if(m_OBUfilecount == 1)
        {
            m_OBUfileNum = OBUTotalCount/1000000;
            if(OBUTotalCount%1000000 > 0 ) m_OBUfileNum++;
        }

        logstr = QString("File OBU Header:영업소-%1, 당일날짜-%2, 파일번호-%3, 전체건수-%4, 파일건수-%5")
                .arg(ManagerID,4,10,QChar('0')).arg(OBUDate,8,10,QChar('0'))
                .arg(FileNum).arg(OBUTotalCount).arg(OBUFileCount);
        log->write(logstr,LOG_NOTICE); qDebug() << logstr;

    }

    int loopcount = 10;
    for(int loopindex=0; loopindex < loopcount; loopindex)
    {
        int obuvehiclelen = 13;
        int maxread = obuvehiclelen * 1000; // obudata(13, obu8/number5) * 1000;
        QByteArray obudata = OBUfile->read(maxread);

        int count = obudata.size()/obuvehiclelen;
        int obunumlen = 8;
        int vehiclenumlen = 5;
        QList<OBUData> datalist;

        for(int index=0, findex=0 ; index < count; index++)
        {
            OBUData data;
            data.obunum = obudata.mid(findex,obunumlen); findex += obunumlen;
            data.obuvehicle = obudata.mid(findex,vehiclenumlen); findex += vehiclenumlen;
            datalist.append(data);
        }

        if( count > 0)
        {
            if(!insertOBUdata(OBUdbname,OBUtablename,datalist))
            {
                logstr = QString("Failed Update OBU Data(file:%1, %2)").arg(strobufilename).arg(m_OBUfdataindex);
                log->write(logstr,LOG_NOTICE);
            }

            m_OBUfdataindex += obudata.size();
        }

        if(obudata.size() <  maxread)
        { // end of file
            m_OBUfilecount++;
            OBUfile->close();
            delete OBUfile;
            OBUfile = NULL;

            logstr = QString("Close OBU File(%1)!!").arg(strobufilename);
            if(plogdlg0 != NULL) { plogdlg0->logappend(LogDlg::logcenter,logstr);}
            log->write(logstr,LOG_NOTICE);

            if(m_OBUfilecount > m_OBUfileNum)
            {
                logstr = QString("OBU DB Update Success!!");
                if(plogdlg0 != NULL) { plogdlg0->logappend(LogDlg::logcenter,logstr);}
                log->write(logstr,LOG_NOTICE);

                emit OBUUpdateResult(m_OBUcenterid,OBUUpdateNo,true,OBUfileName);

                bOBUupdate = false;
            }
            break;
        }

    }

    brtn = true;

    return brtn;
}

bool dbmng::checkOBUdb(QString strdbname)
{
    if(!m_db.isOpen())
    {
        if( !m_db.open())
        {
            return false;
        }
    }

    //check DB
    QSqlQuery query(m_db);
    QString querystring;
    querystring = "show databases like '" + strdbname +"'";
    query.exec(querystring);
    if(query.next()) //  DB check;
    {
        qDebug() << strdbname <<"database is already";
    }
    else
    {
        return false;
    }

    return true;
}

bool dbmng::createOBUdb(QString strdbname)
{
    bool brtn = true;

    if(m_db.isOpen())
    {
        QSqlQuery query(m_db);
        QString querystring = "create database " + strdbname;
        if(query.exec(querystring)){}
        else
        {
            // fail
            qDebug() << "create " + strdbname + " database fail";
            brtn = false;
        }
    }
    else brtn = false;

    return brtn;
}

bool dbmng::setOBUtable()
{
    bool brtn = true;

    if(m_db.isOpen())
    {
        QSqlQuery query(m_db);
        QString querystring;

        //connection check;
        querystring = QString("select table_name from information_schema.tables where table_schema = '%1' order by table_name desc ")
                .arg(OBUdbname);


        if(query.exec(querystring))
        { //connection ok

            if( query.next())
            {
                QString tablename = query.value(0).toString();
                if(tablename.isNull() || tablename.isEmpty())
                {
                    brtn = false;
                    OBUMatchtable = "";
                }
                else
                {
                    OBUMatchtable = QString(tablename.data(),tablename.size());
                }
            }
            else brtn = false;
        }
        else
        { //disconnection
            qDebug() << QString("Failed checkOBUtable(%1)").arg(querystring);
            brtn = false;
        }

    }
    else brtn = false;

    return brtn;
}

bool dbmng::checkOBUtable(QString strdbname, QString strtablename)
{
    bool brtn = true;

    if(m_db.isOpen())
    {
        QSqlQuery query(m_db);
        QString querystring;

        //connection check;
        querystring = QString("select count(table_name) from information_schema.tables where table_name='%1' and table_schema = '%2'")
                .arg(strtablename).arg(strdbname);


        if(query.exec(querystring))
        { //connection ok

            if( query.next())
            {
                int count = query.value(0).toInt();
                //qDebug() << QString("checkNcreatetable Success(count:%1|%2)").arg(count).arg(querystring);
                if( count == 0)
                {
                    brtn = false;
                }
            }
            else brtn = false;
        }
        else
        { //disconnection
            qDebug() << QString("Failed checkOBUtable(%1)").arg(querystring);
            brtn = false;
        }

    }
    else brtn = false;

    return brtn;
}

bool dbmng::insertOBUdata(QString strdbname,QString strtablename,QList<dbmng::OBUData> obulist)
{
    bool brtn = true;

    if(!checkOBUtable(strdbname,strtablename))
    {
        if(!createOBUtable(strdbname,strtablename))
        {
            return false;
        }
    }

    if(m_db.isOpen())
    {

        QDateTime Indextime = QDateTime::currentDateTime();
        QString querystring;
        //QString strtablename1 = QString("%1_%2").arg(strtablename).arg(Indextime.toString("yyyyMMdd"));

        querystring = QString("insert into %1.%2 (Index1,OBUNumber,VehicleNumber) values ").arg(strdbname).arg(strtablename);

        int count = obulist.count();

        for(int index=0; index < count; index++)
        {
            //Index varchar
            if(index != 0 ) querystring += " , ";
            QString strindex = QString("%1-%2").arg(Indextime.toString("yyyyMMddHHmmss")).arg(index);
            querystring += QString(" ( '%1', '%2', '%3' )").arg(strindex)
                    .arg(QString(obulist[index].obunum.toHex())).arg(QString(obulist[index].obuvehicle.toHex()));
        }

        QSqlQuery query(m_db);
        if(query.exec(querystring))
        {
            qDebug() <<  QString("insertOBUdata Success(%1.%2)").arg(strdbname).arg(strtablename);
            brtn = true;
        }
        else
        {
            qDebug() <<  QString("insertOBUdata Fail(%1.%2)").arg(strdbname).arg(strtablename);
            brtn = false;

        }
    }
    else brtn = false;

    return brtn;
}

QByteArray dbmng::searchOBUdata(QByteArray obunum)
{
    QByteArray vehiclenum;

    if(m_db.isOpen())
    {
        //when OBU update, Stop OBUMatching
        if(bOBUupdate)
        {
            return vehiclenum;
        }

        if(OBUMatchtable.isNull() || OBUMatchtable.isEmpty() )
        {
            return vehiclenum;
        }

//        if(!checkOBUtable(OBUdbname,OBUMatchtable))
//        {
//           return vehiclenum;
//        }

        sqlmutex.lock();
        QSqlQuery query(m_db);
        QString querystring;
        QString strtablename1 = QString("%1.%2").arg(OBUdbname).arg(OBUMatchtable);

        //connection check;
        querystring = QString("select  VehicleNumber from %1 where OBUNumber = '%2' ")
                .arg(strtablename1).arg(QString(obunum));


        if(query.exec(querystring))
        { //connection ok

            int numRows = query.numRowsAffected();
            if( numRows >= 1)
            {
                if( query.next())
                {
                    QString strdata = query.value(0).toString();
                    vehiclenum = QByteArray::fromHex(strdata.toUtf8());
                    if( numRows > 1)
                    {
                        QString logstr = QString("OBU : %1(count: %2)").arg(QString(obunum.toHex())).arg(numRows);
                        log->write(logstr,LOG_NOTICE);
                    }

                }
            }            
        }
        else
        { //disconnection
            qDebug() << QString("Failed searchOBUdata(%1)").arg(querystring);
        }
        sqlmutex.unlock();

    }

    return vehiclenum;
}

bool dbmng::createOBUtable(QString strdbname,QString strtablename)
{
    bool brtn = true;

    if(m_db.isOpen())
    {
        QSqlQuery query(m_db);
        QString querystring;

        querystring = " create table " + strdbname +"." + strtablename + " (";
        //sdw //2016/09/22  insert index
        querystring += "	Index1 varchar(50) NULL, ";
        querystring += "	OBUNumber varchar(16) NOT NULL, ";
        querystring += "	VehicleNumber varchar(10) NULL, ";
        querystring += "    INDEX idx1 (Index1) )";

        if(query.exec(querystring))
        {
            //"obudb database, obudata table 생성 성공"
            qDebug() <<  QString("create table Success(%1.%2)").arg(strdbname).arg(strtablename);
            brtn = true;
        }
        else
        {
            //"create obudata 실패"
            qDebug() <<  QString("create table Fail(%1.%2)").arg(strdbname).arg(strtablename);
            //disconnect();
            brtn = false;

        }
    }
    else brtn = false;

    return brtn;
}
