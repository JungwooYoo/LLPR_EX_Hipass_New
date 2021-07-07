#include "recogthread.h"
#include <dlfcn.h>
#include "commonvalues.h"

recogthread::recogthread(int loglevel,QThread *parent) :
    QThread(parent)
{
    brun = false;
    log = new Syslogger(this,"recog",true,loglevel);
    m_loglevel = loglevel;
    //sdw 160912 log->write("Recogthread Create",log->HIGHLEVEL);
    //pointgray  MSB
    //                   8,9,7,6,10,11,5,12 shift
    //arShift = new int[8]{4,5,3,2,6,7,1,8};
    rawdir = QApplication::applicationDirPath() + "/rawimage/";
    QDir mdir(rawdir);
    if(!mdir.exists())
    {
         mdir.mkpath(rawdir);
         qDebug() <<"Create Directory:" << rawdir;
    }
    irecoglicense = ALL_LICENSE_OK;

    codec = QTextCodec::codecForName("eucKR");

}

recogthread::~recogthread()
{
    delete lpr;
    //if (lpr) lpr->LPR_Release();

    delete alpr;
}

void recogthread::SetLogLevel(int loglevel)
{
    log->m_loglevel = loglevel;
    m_loglevel = loglevel;
}

void recogthread::init()
{
    initRecog(commonvalues::cameraSys[0].cam_image_width,commonvalues::cameraSys[0].cam_image_height);

    //sdw //2017/05/07
    alpr = new ALPR_api(1,0);
    int ivalid = alpr->GetSWLicenseStatus();
    QString logstr;
    if( ivalid == 0)  { logstr = QString("ALPR License is valid"); }
    else  { logstr = QString("ALPR License is not valid(%1)").arg(ivalid); irecoglicense |= recogthread::ALPR_LICENSE_FAIL;}
    log->write(logstr,LOG_ERR);
    qDebug() << logstr;

    //alpr version
    logstr = QString("ALPR Version is %1").arg(QString::fromStdString(alpr->GetVersionString()));
    log->write(logstr,LOG_INFO);
    qDebug() << logstr;
}


void recogthread::stop()
{
    brun = false;    

}
void recogthread::run()
{
     QString logstr;
     brun = true;
     int rawcount;
     Recogresult recogres;
     memset(&recogres,0, sizeof(struct Recogresult));

     log->write("Recogthread Init",LOG_NOTICE);
     init();

     log->write("Recogthread Start",LOG_NOTICE);

     while( brun )
     {
         try{
             if( !procindex.isEmpty()) //check capture command
             {
                qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << "Leave Recog Start";
                int count = procindex.size();
                for(int i=0; i < count; i++)
                {
                    VehcileIndex vindex = procindex.takeFirst();
                    int index = vindex.index;
                    int channel = vindex.channel;

                    LogDlg *plogdlg;
                    if(channel == 1)
                        plogdlg = plogdlg1;
                    else
                        plogdlg = plogdlg0;

                    QDateTime prerecogtime = QDateTime::currentDateTime();

                    //if(plogdlg != NULL) { plogdlg->logappend(logdlg::logrecog,"Start Recog " + QString::number(commonvalues::vehicledatalist[index].seq)); }
                    qDebug() << "Recog Start" << commonvalues::vehicledatalist[channel][index].rawImagelen;
                    log->write(QString("Recog Start %1").arg(commonvalues::vehicledatalist[channel][index].seq),LOG_NOTICE);

                    int ret = doRecognition(channel,index,&recogres);

                    QString yeongNum = CheckYeong(&recogres);
                    memcpy(recogres.szPlateNumber, yeongNum.toUtf8().data(), yeongNum.toUtf8().length());

                    QDateTime afterrecogtime = QDateTime::currentDateTime();
                    qint64 diffrecogtime = prerecogtime.msecsTo(afterrecogtime);
                    QString car_recog_num = "";

                    commonvalues::vehicledatalist[channel][index].recogresult = ret;

                    if( ret > 0)
                    {
                           car_recog_num = QString::fromUtf8(recogres.szPlateNumber);
                    }
                    else
                    {
                        car_recog_num = "xxxxxx";
                    }
                    //sdw 2017/01/10  matching lpdata
                    //qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << "LPDATA Check Enter";
                    QString modVehicleNum = MatchLPDATA(car_recog_num);
                    //qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << "LPDATA Check Leave";

                    if( modVehicleNum != "")
                    {
                        QString logstr = QString("Vehicle Number Change-[%1]->[%2]").arg(car_recog_num).arg(modVehicleNum);
                        log->write(logstr,LOG_NOTICE); qDebug() << logstr;

                        car_recog_num = modVehicleNum;
                    }
                    //sdw leave

                   qDebug() << QString("Recog : ret - %1, car_num - %2, recog time - %3msec, Shift - %4")
                               .arg(ret).arg(car_recog_num).arg(diffrecogtime).arg(recogres.RecogShift);
                   qDebug() << "Recog : plateX-" << recogres.plateX << " plateY - " << recogres.plateY
                            << "plateWidth-" << recogres.plateWidth << " plateHeight - " << recogres.plateHeight;


                    //commonvalues::vehicledatalist[recogimg.index].plateImage = new QImage(plateimg);
                    //commonvalues::vehicledatalist[index].recognum = car_recog_num.toLocal8Bit();
                    strcpy((char*)commonvalues::vehicledatalist[channel][index].recognum,car_recog_num.toLocal8Bit().constData());
                    commonvalues::vehicledatalist[channel][index].recogtime = diffrecogtime;//dblRecogTime;
                    commonvalues::vehicledatalist[channel][index].recogshift = 1;
                    commonvalues::vehicledatalist[channel][index].plate_x = recogres.plateX;
                    commonvalues::vehicledatalist[channel][index].plate_y = recogres.plateY;
                    commonvalues::vehicledatalist[channel][index].plate_width = recogres.plateWidth;
                    commonvalues::vehicledatalist[channel][index].plate_height = recogres.plateHeight;

                    //PlateImage Create
                    if( ret > 0 && commonvalues::vehicledatalist[channel][index].imglen > 0)
                    {
                        QImage qplateImage = QImage(commonvalues::vehicledatalist[channel][index].img,commonvalues::cameraSys[channel].cam_image_width,commonvalues::cameraSys[channel].cam_image_height,QImage::Format_RGB888)
                                .copy(recogres.plateX,recogres.plateY,recogres.plateWidth,recogres.plateHeight);
                        commonvalues::vehicledatalist[channel][index].plateImagelen = qplateImage.byteCount();
                        memcpy(commonvalues::vehicledatalist[channel][index].plateImage,qplateImage.constBits(),commonvalues::vehicledatalist[channel][index].plateImagelen);
                    }
                    commonvalues::vehicledatalist[channel][index].vehicleproc  |= commonvalues::recognition;

                    emit RecogEndSignal(channel,index);

                    if(plogdlg != NULL) { plogdlg->logappend(LogDlg::logrecog,QString("Vehicle Number(%1): %2(%3ms)").arg(commonvalues::vehicledatalist[channel][index].seq).arg(car_recog_num).arg(diffrecogtime));}

                    QString logstr;
                    logstr = QString("vehicle num : %1 , vehicle wait time : %2").arg(car_recog_num).arg(diffrecogtime);
                    log->write(logstr,LOG_NOTICE);
                    logstr = QString("plate x:%1, y:%2, width:%3, height:%4").arg(recogres.plateX).arg(recogres.plateY).arg(recogres.plateWidth).arg(recogres.plateHeight);
                    log->write(logstr,LOG_NOTICE);


                    qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << "Leave Recog End";
                    log->write(QString("Recog End %1").arg(commonvalues::vehicledatalist[channel][index].seq),LOG_NOTICE);


                    //raw save check
                    if(commonvalues::cameraSys[channel].recog_rawsavecount > 0)
                    {
                        QString filename;
                        bool bsaveflag = false;

                        if(commonvalues::cameraSys[channel].recog_rawsavetype == 0 ) //all
                        {
                            bsaveflag = true;
                        }
                        else if(commonvalues::cameraSys[channel].recog_rawsavetype == 1 &&  !(ret > 0) ) //no recog
                        {
                            bsaveflag = true;
                        }
                        else if(commonvalues::cameraSys[channel].recog_rawsavetype == 2 && (ret > 0) )   //recog
                        {
                            bsaveflag = true;
                        }

                        if( bsaveflag)
                        {
                            int imgwidth = commonvalues::cameraSys[channel].cam_image_width;
                            int imgheight = commonvalues::cameraSys[channel].cam_image_height;
                            filename = rawdir + QString("%1_ch%2.raw").arg(rawcount).arg(channel);
                            ImagePtr rawImage = Image::Create(imgwidth,imgheight,0,0,PixelFormat_BayerRG16,commonvalues::vehicledatalist[channel][index].rawImage);
                            //rawImage.SetData(commonvalues::vehicledatalist[channel][index].rawImage,commonvalues::vehicledatalist[channel][index].rawImagelen);
                            rawImage->Save(filename.toStdString().c_str(),RAW);
                            rawcount++;
                            log->write(QString("Raw File Save : %1").arg(filename),LOG_NOTICE);
                        }
                        if(rawcount >= commonvalues::cameraSys[channel].recog_rawsavecount) rawcount = 0;

                    }
                    log->write("---------------",LOG_NOTICE);
                    // recog license check
                    int recogcount = commonvalues::vehicledatalist[channel][index].seq;
                    if(recogcount%10 == 0)
                    {
                        int istatus = alpr->GetSWLicenseStatus();
                        if( istatus != 0)
                        {
                            logstr = QString("ALPR License is not valid(%1)").arg(istatus);
                            qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ")
                                                    << logstr;
                            log->write(logstr,LOG_NOTICE);
                            irecoglicense |= ALPR_LICENSE_FAIL;
                        }

                        istatus = lpr->GetSWLicenseStatus();
                        if( istatus != 0)
                        {
                            logstr = QString("iLPR License is not valid(%1)").arg(istatus);
                            qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ")
                                                    << logstr;
                            log->write(logstr,LOG_NOTICE);
                            irecoglicense |= iLPR_LICENSE_FAIL;
                        }
                    }
                    msleep(30);
                }
            }
            else   msleep(30);

            commonvalues::systemcheck |= commonvalues::sysrecognition;
         }
         catch( ... )
         {
             logstr = QString("--------- Error Recogthread Thread -----------");
             log->write(logstr,LOG_ERR);            qDebug() << logstr;
             msleep(30);
         }
     }
     //sdw 2017/05/07
     lpr->LPR_Release();

     logstr = QString("Recogthread Stop");
     log->write(logstr,LOG_ERR);

}

void recogthread::initRecog(int width,int height)
{
    QString logstr;

    for(int ch=0; ch < 2; ch++)
    {
        image_width[ch] = width;
        image_height[ch] = height;

        //init value
        recog_plate_sy[ch] = 0;
        recog_plate_ey[ch] = height -1;
        recog_iLPRType[ch] = 0;
        recog_iCamView[ch] = Center;
        recog_iShftCount[ch] = 4;
        int shift[8] = {4, 3, 5, 2, 6, 1, 8, 7};
        memcpy(recog_arShift[ch] ,shift, sizeof(shift));
    }
    int sy = 0;
    int ey = height - 1;

    //load config
    recogsetting();

    //sdw 2019/08/19
    //load ilpr so file, create
    void* handle = dlopen("liblpr.so", RTLD_LAZY);
    if(handle == 0) {
        fprintf(stderr, "Load so file failed\n");
        return;
    }

    IPLPR* (*create)();
    void (*destroy)(IPLPR*);

    create = (IPLPR* (*)())dlsym(handle, "create_object");
    destroy = (void (*)(IPLPR*))dlsym(handle, "destroy_object");

    lpr = (IPLPR*)create();

    lpr->LPR_Init(width,height,sy,ey);
    int ivalid = lpr->GetSWLicenseStatus();
    if( ivalid == 0)  { logstr = QString("iLPR License is valid");  }
    else  { logstr = QString("iLPR License is not valid(%1)").arg(ivalid); irecoglicense |= recogthread::iLPR_LICENSE_FAIL;}

    qDebug() << logstr;
    log->write(logstr,LOG_ERR);

    //ilpr version
    char cversion[128];
    lpr->IP_LPR_GetVersion(cversion);
    QString strversion(cversion);
    logstr = QString("iLPR Version is %1").arg(strversion);
    log->write(logstr,LOG_INFO);
    qDebug() << logstr;



}

//void recogthread::RecogImageInsert(RecogImage recogimge)
//{
//    Recogimg.enqueue(recogimge);
//    log->write(QString("Recog Enqueue : %1").arg(recogimge.index),LOG_DEBUG);
//}

void recogthread::recogsetting()
{
    for(int ch=0; ch < 2; ch++)
    {
        recog_plate_sx[ch] = commonvalues::cameraSys[ch].recog_plateroi_sx;
        recog_plate_sy[ch] = commonvalues::cameraSys[ch].recog_plateroi_sy;
        recog_plate_ey[ch] = commonvalues::cameraSys[ch].recog_plateroi_sy + commonvalues::cameraSys[ch].recog_plateroi_height;
        recog_plate_ex[ch] = commonvalues::cameraSys[ch].recog_plateroi_sx + commonvalues::cameraSys[ch].recog_plateroi_width;
        recog_iLPRType[ch] = 0; //campus
        recog_iCamView[ch] = commonvalues::cameraSys[ch].recog_vehicleposition;
        recog_mode[ch] = (RecogMode)commonvalues::cameraSys[ch].recog_recognition_mode;
        recog_iShftCount[ch] = commonvalues::cameraSys[ch].recog_recogrepeattime;
        memcpy(recog_arShift ,commonvalues::cameraSys[ch].recog_recogseq, sizeof(commonvalues::cameraSys[ch].recog_recogseq));

        log->write(QString("RecogSetting(ch%1) - sy : %2 , ey : %3 , lprtype : %4 , position : %5 , Shiftcount : %6")
                   .arg(ch).arg(recog_plate_sy[ch]).arg(recog_plate_ey[ch]).arg(recog_iLPRType[ch]).arg(recog_iCamView[ch]).arg(recog_iShftCount[ch]),LOG_NOTICE);

    }
}

int recogthread::doRecognition(int channel, int index,Recogresult *recogres)
{
    int ret = 0;

    RecogMode mode = recog_mode[channel];

    switch(mode)
    {
        case ALPR_MODE:
            ret = Recog_ALPR(channel,index,recogres);
            break;
        case iLPR_MODE:
            ret = Recog_iLPR(channel,index,recogres);
            break;
        case ALPR_iLPR_MODE:
            ret = Recog_ALPR(channel,index,recogres);
            if( ret <= 0) ret = Recog_iLPR(channel,index,recogres);
            break;
        case iLPR_ALPR_MODE:
            ret = Recog_iLPR(channel,index,recogres);
            if( ret <= 0) ret = Recog_ALPR(channel,index,recogres);
            break;
        case DUAL_ALPR_iLPR_MODE:
            ret = Recog_ALPR_iLPR(channel,index,recogres);
            break;
        case DUAL_iLPR_ALPR_MODE:
            ret = Recog_iLPR_ALPR(channel,index,recogres);
            break;
        default :
            ret = 0;

    }

   return ret;
}

//return :  0 : 미인식, 1 : 인식
int recogthread::Recog_ALPR(int channel, int index,Recogresult *recogres,int img_sx, int img_sy,int img_width, int img_height)
{
    QString logstr = "ALPR Recognition Start";
    qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << logstr;
    log->write(logstr,LOG_INFO);

    int ret;

    QDateTime pretime = QDateTime::currentDateTime();

    //recogthread::Recogresult recogres;
    if(commonvalues::vehicledatalist[channel][index].rawImagelen > 0 )
    {
        int width = commonvalues::cameraSys[channel].cam_image_width;
        int height = commonvalues::cameraSys[channel].cam_image_height;

//        unsigned char *rawimg = (unsigned char*)malloc(commonvalues::vehicledatalist[index].rawImage.GetDataSize());
        unsigned char *img = commonvalues::vehicledatalist[channel][index].rawImage;
        unsigned char *orgimg;

        //crop image
        if( img_sx != -1 && img_sy != -1 && img_width != -1 && img_height != -1)
        {
            orgimg = (unsigned char*)malloc(img_width*img_height*2*sizeof(unsigned char) + 2);
            int imgline = 2 * width;
            int orgimgline = 2 * img_width;
            for(int i=0; i < img_height; i++)
            {
                unsigned char *pcrop = img + (( img_sy + i) * imgline ) + ( img_sx * 2 );
                unsigned char *porgimgstart = orgimg + ( i * orgimgline);
                memcpy(porgimgstart,pcrop,orgimgline);
            }
            qDebug() << QString("crop image(ch%5) :sx-%1, sy-%2, width-%3, height-%4")
                        .arg(img_sx).arg(img_sy).arg(img_width).arg(img_height).arg(channel);
            width = img_width;
            height = img_height;
        }
        else
        { //full image
            orgimg = img;
        }

        std::vector<LPDataInfo> batch_imginfo;
        LPDataInfo oneInfo;
        oneInfo.imgType = 2;  // 2: PointGrey camera RAW
        oneInfo.TL.x = -100;
        oneInfo.TL.y = -100;
        oneInfo.TR.x = -100;
        oneInfo.TR.y = -100;
        oneInfo.BL.x = -100;
        oneInfo.BL.y = -100;
        oneInfo.BR.x = -100;
        oneInfo.BR.y = -100;
        oneInfo.bPosGT = false;
        oneInfo.noGT = true;
        oneInfo.imgWidth = width;
        oneInfo.imgHeight = height;
        oneInfo.rawData = orgimg;

        if(oneInfo.rawData == NULL)
            return -1;
        batch_imginfo.push_back(oneInfo);

        QDateTime imgtime = QDateTime::currentDateTime();
                qDebug() << QString("image load(%1ms)")
                            .arg(pretime.msecsTo(imgtime));



        std::vector<std::vector<LPResult>> batchresultList = alpr->RecognizeLP(batch_imginfo);
        std::vector<LPResult> resultList = batchresultList[0];
        QDateTime aftertime = QDateTime::currentDateTime();
        qDebug() << QString("ALPR Recog time(ch%1) : %2").arg(channel).arg(pretime.msecsTo(aftertime));

        LPResult res;
        if( resultList.size() > 0 )
        {
          int index = CheckVehicleNumber(resultList);
          if( index < 0 || index >= (unsigned int)resultList.size())
              index = 0;

          qDebug() << QString("Select VehicleNumber %1").arg(index);
          res = resultList[index];

        }

        //recogthread::Recogresult recogres;
        if( img_sx != -1 && img_sy != -1 && img_width != -1 && img_height != -1)
        {
            recogres->plateX = res.roi.x + img_sx;
            recogres->plateY = res.roi.y + img_sy;
            recogres->plateWidth = res.roi.width;
            recogres->plateHeight = res.roi.height;
            free(orgimg);
        }
        else
        {
            recogres->plateX = res.roi.x;
            recogres->plateY = res.roi.y;
            recogres->plateWidth = res.roi.width;
            recogres->plateHeight = res.roi.height;
        }

        qDebug() << QString("roi(ch%5)[x,y,w,h] : %1, %2, %3, %4").arg(recogres->plateX).arg(recogres->plateY).arg(recogres->plateWidth).arg(recogres->plateHeight).arg(channel);
        qDebug() << QString("Recognition result(ch%5) : %1, %2, %3, %4").arg(QString::fromUtf8(res.strRegion.c_str())).arg(QString::fromUtf8(res.strType.data()))
                    .arg(QString::fromUtf8(res.strUsage.data())).arg(QString::fromUtf8(res.strMainNumber.data())).arg(channel);

        //recogthread::Recogresult recogres;
        memset(recogres->szPlateNumber,0,sizeof(recogres->szPlateNumber));
        sprintf(recogres->szPlateNumber, "%s%s%s%s", res.strRegion.data(), res.strType.data(), res.strUsage.data(), res.strMainNumber.data());

        if (res.strType.empty() || res.strUsage.empty() || res.strMainNumber.empty())
            ret = 0;
        else if( strchr(recogres->szPlateNumber,'x') != NULL )
            ret = 0;
        else
            ret = 9;
    }
    else ret = 0;

    //sdw 2017/08/09
    //부분인식체크
    if( ret > 0 )
    {
        QString car_recog_num = codec->toUnicode(recogres->szPlateNumber);
        int index = car_recog_num.indexOf('x');
        if( index != -1 )
        {
            QString logstr = QString("ALPR result :  %1   roi[x,y,w,h] : %2, %3, %4, %5").arg(car_recog_num)
                    .arg(recogres->plateX).arg(recogres->plateY).arg(recogres->plateWidth).arg(recogres->plateHeight);
            log->write(logstr,LOG_INFO);
            ret = 0;
        }
    }

    return ret;
}

//void recogthread::Recog_ALPR_Result(int rtn, recogthread::Recogresult recogres)
//{
//    memcpy(&m_recogres,&recogres,sizeof(Recogresult));
//    alprreturn = rtn;
//    balpr_result = true;
//}

//return :  0 : 미인식, 1 : 인식
int recogthread::Recog_iLPR(int channel,int index,Recogresult *recogres,int img_sx, int img_sy, int img_ex, int img_ey)
{
    QDateTime pretime = QDateTime::currentDateTime();


    int recog_sx = recog_plate_sx[channel];
    int recog_sy = recog_plate_sy[channel];
    int recog_ex = recog_plate_ex[channel];
    int recog_ey = recog_plate_ey[channel];
    if( img_sx != -1 && img_sy != -1 && img_ex != -1 && img_ey != -1)
    {
        recog_sx = img_sx;
        recog_sy = img_sy;
        recog_ex = img_ex;
        recog_ey = img_ey;
    }

    QString logstr = "iLPR Recognition Start";
    qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << logstr;
    log->write(logstr,LOG_INFO);

    recogres->fileIndex = 0;
    recogres->iSaveLog = commonvalues::recogSaveLog;

    //2016/11/13 -> 2017/04/21
    //ret = 0(미인식), 1(정인식,부분인식)
    int ret = lpr->ReadPlateNumber_Data(	//INPUT PARAMS
                                0,			//0: PointGrey, 1: IMI, 2: Smart
                                commonvalues::vehicledatalist[channel][index].rawImage, //recogimg.img,//recogimg.img.GetData(),		//16bit raw image data
                                image_width[channel], image_height[channel],		//image size
                                recog_sx, recog_sy,recog_ex, recog_ey,				//top, down limit
                                recog_iLPRType[channel],				//0: campus version, 1: highway version
                                recog_iCamView[channel],			//0: front, 1: passenger-side, 2: driver-side
                                1,				//1
                                recog_iShftCount[channel],
                                recog_arShift[channel],

                                //OUTPUT PARAMS
                                &recogres->plateX, &recogres->plateY, &recogres->plateWidth, &recogres->plateHeight,	//plate location
                                &recogres->plateType,		//plate color type: (1: dark char, bright background) (-1: bright char, dark background)
                                &recogres->plateIntensity,	//plate region's intensity
                                recogres->szPlateNumber,	//plate number
                                &recogres->dblRecogTime,	//recognition time
                                //&recogres->RecogShift,      //recog result shift index
                                recogres->fileIndex,			//0
                                recogres->iSaveLog,			//0
                                recogres->szLogFile	//""
                                );	
    QDateTime aftertime = QDateTime::currentDateTime();
    qDebug() << QString("iLPR Recog time : %1").arg(pretime.msecsTo(aftertime));
//    if( ret > 0 )  //인식 시 plate 좌표수정
//    {
//           int platey = recogres->plateY;
//           recogres->plateY = platey + recog_sy;
//           qDebug() << QString("roi change : %1 -> %2").arg(platey).arg(recogres->plateY);
//    }
    //sdw //2016/10/11 QTextCodec -> 전역
    //QTextCodec * codec = QTextCodec::codecForName("eucKR");
    QString car_recog_num = codec->toUnicode(recogres->szPlateNumber);
    memset(recogres->szPlateNumber,0,sizeof(recogres->szPlateNumber));
    memcpy(recogres->szPlateNumber, car_recog_num.toUtf8().data(), car_recog_num.toUtf8().length());
    //sdw 2017/08/09
    logstr = QString("iLPR result[%2] :  %1   roi[x,y,w,h] : %3, %4, %5, %6").arg(car_recog_num).arg(ret)
            .arg(recogres->plateX).arg(recogres->plateY).arg(recogres->plateWidth).arg(recogres->plateHeight);
    qDebug() << logstr;
    //부분인식체크
    if( ret > 0 )
    {
        int index = car_recog_num.indexOf('x');
        if( index != -1 )
        {

            log->write(logstr,LOG_INFO);
            ret = 0;
        }
    }


    return ret;
}

//return :  0 : 미인식, 1 : 인식
int recogthread::Recog_ALPR_iLPR(int channel,int index, recogthread::Recogresult *recogres)
{
    Recogresult lrecogres1, lrecogres2;
    memset(&lrecogres1,0, sizeof(struct Recogresult));
    memset(&lrecogres2,0, sizeof(struct Recogresult));

    bool bthreenum=false;
    int ret = 0, ret1 = 0, ret2 = 0;

    ret1 = Recog_ALPR(channel,index,&lrecogres1);
    if( ret1 > 0)
    {
        char fchar = lrecogres1.szPlateNumber[0];
        if( fchar >= '0' && fchar <= '9' ) //신형번호판
        {
            QDateTime prerecogtime = QDateTime::currentDateTime();          

            //recog plate area
            ret2 = Recog_iLPR(channel,index,&lrecogres2);

            if( ret2 > 0 )
            {
                bthreenum=true;
                for(int i=0; i < 3; i++)  //check front three charator
                {
                    //fchar = lrecogres2.szPlateNumber[i];
                    qDebug() << QString("check[%1] : %2").arg(i).arg(fchar);
                    if( fchar < '0' || fchar > '9' )
                    {
                        bthreenum = false;
                       // qDebug() << QString("three Number is False");
                        break;
                    }
                }
            }

            QDateTime afterrecogtime = QDateTime::currentDateTime();
            qint64 diffrecogtime = prerecogtime.msecsTo(afterrecogtime);

            QString logstr = QString("Three Number Check[%1] : %2/%3(%4ms)")
                        .arg(bthreenum ? "True" : "False").arg(lrecogres1.szPlateNumber).arg(lrecogres2.szPlateNumber).arg(diffrecogtime);
            log->write(logstr,LOG_NOTICE); qDebug() << logstr;

        }
    }

    if(bthreenum)
    {
        memcpy(recogres,&lrecogres2,sizeof(Recogresult));
        ret = ret2;
    }
    else
    {
        memcpy(recogres,&lrecogres1,sizeof(Recogresult));
        ret = ret1;
    }

    return ret;
}

int recogthread::Recog_iLPR_ALPR(int channel, int index, recogthread::Recogresult *recogres)
{
    Recogresult lrecogres1, lrecogres2;
    memset(&lrecogres1,0, sizeof(struct Recogresult));
    memset(&lrecogres2,0, sizeof(struct Recogresult));

    bool bthreenum=false;
    int ret = 0, ret1 = 0, ret2 = 0;

    ret1 = Recog_iLPR(channel,index,&lrecogres1);
    if( ret1 > 0 )
    {
        bthreenum=true;
        for(int i=0; i < 3; i++)  //check front three charator
        {
            int fchar = lrecogres1.szPlateNumber[i];
            //qDebug() << QString("check[%1] : %2").arg(i).arg(fchar);
            if( fchar < '0' || fchar > '9' )
            {
                bthreenum = false;
                qDebug() << QString("three Number is False");
                break;
            }
        }
    }
    if(bthreenum) //신형3자리번호판은 바로 리턴
    {
        memcpy(recogres,&lrecogres1,sizeof(Recogresult));
        return ret1;
    }

    ret2 = Recog_ALPR(channel,index,&lrecogres2);


    if(ret2 > 0) //ALPR 인식 우선
    {
        memcpy(recogres,&lrecogres2,sizeof(Recogresult));
        ret = ret2;
    }
    else
    {
        memcpy(recogres,&lrecogres1,sizeof(Recogresult));
        ret = ret1;
    }

    return ret;
}

//bool recogthread::ConvertBayerRawIntoGray(int channel,int index, int width, int height, cv::Mat &img_8bit, cv::Mat &img_12bit, int shift, int cameraType)
//{
//    bool brtn = true;
//    int rawArea = width * height;

//    Image rawImage(1200,1600,PIXEL_FORMAT_RAW16,BGGR);
//    rawImage.SetData(commonvalues::vehicledatalist[index].rawImage,commonvalues::vehicledatalist[index].rawImagelen);
//    Image convertImage8;
//    Image convertImage16;
//    rawImage.Convert(PIXEL_FORMAT_MONO8 ,&convertImage8);
//    rawImage.Convert(PIXEL_FORMAT_MONO16,&convertImage16);
//    qDebug() << QString("Image8 width : %1 , height : %2 , size : %3").arg(convertImage8.GetRows()).arg(convertImage8.GetCols()).arg(convertImage8.GetDataSize());
//    WORD *bayer_data = new WORD[rawArea];
//    unsigned char * pdata = convertImage16.GetData();
//    memcpy(bayer_data,pdata,convertImage16.GetDataSize());

//    cv::Mat newImg_8bit(height, width, CV_8U, convertImage8.GetData(), width);
//    img_8bit = newImg_8bit.clone();

//    cv::Mat newImg_12bit(height,width, CV_16U, bayer_data, width * sizeof(WORD));
//    img_12bit = newImg_12bit.clone();

//    delete[] bayer_data;

//    return brtn;
//}

void recogthread::RGBToGray(WORD *img, int rawW, int rawH)
{
    int width = rawW;
    int height = rawH;

    int imgWidth = rawW;
    int imgHeight = rawH;

    const int bayerStep = width;
    const int rgbStep = 3 * width;

    int blue = 1;
    int start_with_green = true;

    int i, j, iinc, imax;
    int offset, offset1;
    WORD *bayer = img;
    WORD * rgb = new WORD[width * height * 3];
    WORD * prgb = rgb;

    imax = width * height * 3;

    for (i = width * (height - 1) * 3; i< imax; i++)
    {
        rgb[i] = 0;
    }

    iinc = (width - 1) * 3;

    for (i = (width - 1) * 3; i < imax; i += iinc)
    {
        rgb[i++] = 0;
        rgb[i++] = 0;
        rgb[i++] = 0;
    }

    rgb += 1;
    imgHeight -= 1;
    imgWidth -= 1;

    for (; imgHeight--; bayer += bayerStep, rgb += rgbStep)
    {
        WORD *bayerEnd = bayer + width;

        if (start_with_green)
        {
            rgb[-blue] = bayer[1];
            rgb[0] = bayer[bayerStep + 1];
            rgb[blue] = bayer[bayerStep];
            bayer++;
            rgb += 3;
        }

        if (blue > 0)
        {
            for (; bayer <= bayerEnd - 2; bayer += 2, rgb += 6)
            {
                rgb[-1] = bayer[0];
                rgb[0] = bayer[1];
                rgb[1] = bayer[bayerStep + 1];

                rgb[2] = bayer[2];
                rgb[3] = bayer[bayerStep + 2];
                rgb[4] = bayer[bayerStep + 1];
            }
        }
        else
        {
            for (; bayer <= bayerEnd - 2; bayer += 2, rgb += 6)
            {
                rgb[1] = bayer[0];
                rgb[0] = bayer[1];
                rgb[-1] = bayer[bayerStep + 1];

                rgb[4] = bayer[2];
                rgb[3] = bayer[bayerStep + 2];
                rgb[2] = bayer[bayerStep + 1];
            }
        }

        if (bayer < bayerEnd)
        {
            rgb[-blue] = bayer[0];
            rgb[0] = bayer[1];
            rgb[blue] = bayer[bayerStep + 1];
            bayer++;
            rgb += 3;
        }

        bayer -= width;
        rgb -= width * 3;

        blue = -blue;
        start_with_green = !start_with_green;
    }

    int offset1_j_3;

    for (i = 0; i<height; i++)
    {
        offset = i * width;
        offset1 = rgbStep * i;
        for (j = 0; j<width; j++)
        {
            offset1_j_3 = offset1 + j * 3;

            img[offset + j] = (WORD)(((double)(prgb[offset1_j_3] * 0.2 +
                prgb[offset1_j_3 + 1] * 0.4 +
                prgb[offset1_j_3 + 2] * 0.4)));

        }
    }


    delete[]prgb;
}

void recogthread::Make8BitImage(WORD * p12Bit, int rawW, int rawH, int shiftIndex, PIX_TYPE * pData)
{
    //PIX_TYPE pix_val;

    register int x, y;
    int y_w = 0;

    //int iFirst = 4;


    for (y = 0; y < rawH; y++)
    {
        PIX_TYPE * dataRow = (PIX_TYPE *)(pData + y*rawW);
        WORD * rawRow = (WORD *)(p12Bit + y*rawW);

        for (x = 0; x < rawW; x++)
        {
            dataRow[x] = (PIX_TYPE)min((rawRow[y_w + x] >> shiftIndex), 255);
        }
    }

}

QString recogthread::MatchLPDATA(QString strkey)
{
    QString strreturn = "";

    int count = commonvalues::lpdatalist.size();
    if( count > 0)
    {
        QList<QString> uniquekeys = commonvalues::lpdatalist.uniqueKeys();
        foreach( const QString& key ,uniquekeys)
        {
            if( strkey.contains(key) )
            {
                return commonvalues::lpdatalist.value(key);
            }

        }
    }

    return strreturn;
}

//인식모듈 체크
// ALPR_iLPR_MODE or iLPR_ALPR_MODE의 경우에는 하나만 인식이되어도 true
bool recogthread::GetLicenseStatus()
{
    bool bret = false;
    int iret = 0;

    for(int ch = 0 ; ch < commonvalues::cameraChannel; ch++)
    {
        RecogMode mode = recog_mode[ch];

        if( mode == ALPR_MODE || mode == DUAL_ALPR_iLPR_MODE)
        {
            iret = irecoglicense & ALPR_LICENSE_FAIL;
            if( iret == 0)
                bret = true;
        }
        else if( mode == iLPR_MODE || mode == DUAL_iLPR_ALPR_MODE)
        {
            iret = mode & iLPR_LICENSE_FAIL;
            if( iret == 0)
                bret = true;
        }
        else if( mode == ALPR_iLPR_MODE  ||  mode == iLPR_ALPR_MODE )
        {
            iret = irecoglicense & ALPR_LICENSE_FAIL;
            if( iret == 0)
                bret = true;

            iret = irecoglicense & iLPR_LICENSE_FAIL;
            if( iret == 0)
                bret = true;
        }
    }

    return bret;
}

int recogthread::CheckVehicleNumber(std::vector<LPResult> resultList)
{
    unsigned int count = resultList.size();

        if( count < 2 )
            return ( count - 1);  // 1 -> index : 0 , 0 -> -1;

        int sindex = 0;
        int  sarea = 0;
        for(int index = 0; index < count; index++)
        {
            LPResult res = resultList[index];
            qDebug() << QString("cropImage recogvehicle roi[x,y,w,h] : %1,%2,%3,%4").arg(res.roi.x).arg(res.roi.y).arg(res.roi.width).arg(res.roi.height);
            qDebug() << QString("cropImage Recognition result : %1%2%3%4%5")
                        .arg(res.strRegion.data()).arg(res.strType.data()).arg( res.strUsage.data()).arg(res.strMainNumber.data()).arg(res.strCommercial.data());

            if (res.strType.empty() || res.strUsage.empty() || res.strMainNumber.empty()) //unrecog
                  continue;
            if( res.strType.find("x") != std::string::npos  || res.strUsage.find("x") != std::string::npos || res.strMainNumber.find("x") != std::string::npos )
                  continue;

            //plate area check
            int area = res.roi.width * res.roi.width;
            if( area < 10 )
                    area = 0;

            if( sarea < area )
            {
                sindex = index;
                sarea = area;
                qDebug() << QString("Check index : %1").arg(sindex);
            }

        }
        return sindex;

}

QString recogthread::CheckYeong(recogthread::Recogresult *recogres)
{
    QString logstr;
    QString vehicleNumber =  QString::fromUtf8(recogres->szPlateNumber);
    QString vehicleNum = vehicleNumber.mid(0);
    try
    {
        if (vehicleNum.length() > 8)  //부산11가5111영(10/9) '영'자를 제외하고 9자리가 나와야함.
        {
            QString strregion = vehicleNum.mid(0, 2);
            QString strtype = vehicleNum.mid(2, 2);
            QString strmainNum = vehicleNum.mid(vehicleNum.length() - 4);

            //영업용 번호판에 대한 체크 루틴
            if (!vehicleNum.at(0).isNull() && !vehicleNum.at(1).isNull() && !strregion.contains("x"))
            {
                if (strtype.compare("06") == 0 || strtype.compare("11") == 0 || strtype.compare("14") == 0 || strtype.compare("15") == 0)
                {
                    bool ok;
                    int mainNum = strmainNum.toInt(&ok);
                    if (ok)
                    {
                        if (mainNum >= 5000 && mainNum < 9000)
                        {
                            //영업용 번호판 비율 체크
                            float fwidth = (float)recogres->plateWidth;
                            float fheight = (float)recogres->plateHeight;
                            float plateratio = fwidth / fheight;
                            if (plateratio < 3.3)
                            { //영업용 차량  '영'
                                vehicleNumber.append("영");
                                logstr = QString("영업용 차량 체크 : %1 -> %2").arg(vehicleNum).arg(vehicleNumber);
                                log->write(logstr,LOG_INFO); qDebug() << logstr;
                            }
                        }
                    }
                }
            }
        }

    }
    catch (...)
    {
        logstr = QString("인식결과 체크(영) 에러");
        log->write(logstr,LOG_ERR); qDebug() << logstr;

    }

    return vehicleNumber;

}
