#include "autoiris.h"
#include "commonvalues.h"

autoiris::autoiris(int channel ,spinview *cam,int loglevel, QThread *parent) :
    QThread(parent)
{
    camera = cam;
    log = new Syslogger(this,"autoiris",true,loglevel);
    m_loglevel = loglevel;
    m_channel = channel;

    m_datecheck = QDateTime::currentDateTime().toString("dd");

    gb_AutoIrisUse = false;

    gi_StandardBr = 60;
    gi_BrRange = 5;
    gi_BrInterval = 1000;
    gi_BrAvgCount = 5;
    gf_MaxShutter = 62.0;
    gf_MinShutter = 0.0;
    gf_ShutterChangeStep = 1.0;
    gf_MaxGain = 10.0;
    gf_MinGain = 0.0;
    gf_GainChangeStep = 1.0;

    gi_BrSum = 0;
    gi_BrGetCount = 0;
    gi_BrAvg = 0;

    gi_TargetBr = gi_StandardBr;
    gi_CurrentBr = 0;
    //gi_CurrentShtter = 62;
    gf_CurrentShtter = commonvalues::autoirisinfo[m_channel].autoiris_lastshutter;
    gf_CurrentGain = commonvalues::autoirisinfo[m_channel].autoiris_lastgain;
    IrisState = None;

    gi_CurrentPlateBr = 0;
    gb_ReflectionMode = true;
    gb_BacklightMode = true;
    gi_MaxPlateBr = 200;
    gi_MinPlateBr = 30;
    gi_MaxTargetBr = 200;
    gi_MinTargetBr = 10;
    gi_BrChangeStep = 10;
    LightState = Normal;
    PlateCalMode = PlateAvgBr;

    gb_NoRecogMode = true;
    gi_NoRecogMaxBr = 200;
    gi_NoRecogMinBr = 30;

    gb_LighOnOff = false;
    gf_LightShutter = 1000;
    gf_LightOnGain = 10.0;
    gf_LightOffGain = 0.0;
    gb_24HLightOn = false;

    gb_DebugMode = false;

    m_bcamerarestart = false;
}

autoiris::~autoiris()
{
    log->deleteLater();
}

void autoiris::SetLogLevel(int loglevel)
{
    log->m_loglevel = loglevel;
    m_loglevel = loglevel;
}

void autoiris::init()
{
    gb_AutoIrisUse = commonvalues::autoirisinfo[m_channel].autoiris_use;

    if(gi_StandardBr != commonvalues::autoirisinfo[m_channel].autoiris_standardbr) gi_TargetBr = commonvalues::autoirisinfo[m_channel].autoiris_standardbr;
    gi_StandardBr = commonvalues::autoirisinfo[m_channel].autoiris_standardbr ;
    gi_BrRange = commonvalues::autoirisinfo[m_channel].autoiris_brrange ;
    gi_BrInterval = commonvalues::autoirisinfo[m_channel].autoiris_brinterval;
    gi_BrAvgCount = commonvalues::autoirisinfo[m_channel].autoiris_bravgcount;
    gf_MaxShutter = commonvalues::autoirisinfo[m_channel].autoiris_maxshutter;
    gf_MinShutter = commonvalues::autoirisinfo[m_channel].autoiris_minshutter;
    gf_ShutterChangeStep = commonvalues::autoirisinfo[m_channel].autoiris_shutterstep;
    gf_MaxGain = commonvalues::autoirisinfo[m_channel].autoiris_maxgain;
    gf_MinGain = commonvalues::autoirisinfo[m_channel].autoiris_mingain;
    gf_GainChangeStep = commonvalues::autoirisinfo[m_channel].autoiris_gainstep;

    gb_ReflectionMode = commonvalues::autoirisinfo[m_channel].autoiris_reflection;
    gb_BacklightMode = commonvalues::autoirisinfo[m_channel].autoiris_backlight;
    gi_MaxPlateBr = commonvalues::autoirisinfo[m_channel].autoiris_maxplatebr ;
    gi_MinPlateBr = commonvalues::autoirisinfo[m_channel].autoiris_minplatebr;
    gi_MaxTargetBr = commonvalues::autoirisinfo[m_channel].autoiris_maxbr;
    gi_MinTargetBr = commonvalues::autoirisinfo[m_channel].autoiris_minbr;
    gi_BrChangeStep = commonvalues::autoirisinfo[m_channel].autoiris_brstep;
//    LightState = Normal;
    PlateCalMode = (PlateCalModeList)commonvalues::autoirisinfo[m_channel].autoiris_reflectbackmode;

    gb_NoRecogMode = commonvalues::autoirisinfo[m_channel].autoiris_norecog;
    gi_NoRecogMaxBr = commonvalues::autoirisinfo[m_channel].autoiris_norecogmaxbr;
    gi_NoRecogMinBr = commonvalues::autoirisinfo[m_channel].autoiris_norecogminbr;

    gf_LightShutter = commonvalues::autolightnfo[m_channel].light_shutter;
    gf_LightOnGain = commonvalues::autolightnfo[m_channel].light_ongain;
    gf_LightOffGain = commonvalues::autolightnfo[m_channel].light_offgain;
    gb_24HLightOn = commonvalues::autolightnfo[m_channel].light_24h;

    gb_DebugMode = commonvalues::autoirisinfo[m_channel].autoiris_debug;
}

void autoiris::run()
{
    unsigned int valueA, valueB;
    //bool automode;
    //bool onoff;
    int lightckeckcount=0;

    QString logstr = QString("autoiris thread(ch%1) start").arg(m_channel);
    log->write(logstr,LOG_ERR); qDebug() << logstr;

    brun = true;
    while(brun)
    {
        try
        {
            //br  ??????
            if(camera->GetBright(&valueA))
            {
                //qDebug() << "BR :" << valueA;
                BR_Input((int)valueA);
                //current br
                commonvalues::currentvalue[m_channel].cur_br = valueA;

            }
            else qDebug() << "Autoiris GetBright Fail";
            //?????? ????????? ?????? ?????? ??????
            if(!AutoIrisImg.isEmpty())
            {
                queuemutex.lock();
                AutoIrisImageStruct daiis = AutoIrisImg.dequeue();
                queuemutex.unlock();

                if( daiis.inputtype == 0 )
                {
                    PlateImage_Input(daiis.img,daiis.width,daiis.height,daiis.irisst);
                }
                else if( daiis.inputtype == 1)
                {
                    VehicleImage_Input(daiis.img,daiis.width,daiis.height,daiis.irisst);
                }

            }
        }
        catch( ... )
        {
            logstr = QString("--------- Error Autoiris Action(ch%1) -----------").arg(m_channel);
            log->write(logstr,LOG_ERR);            qDebug() << logstr;
        }

        try
        {
            //camera retstart => camera setting
            if(m_bcamerarestart)
            {
                m_bcamerarestart = false;
                BrSetEvent(gf_CurrentShtter, gf_CurrentGain); //, gb_LighOnOff);
                QString logstr = QString("[Camera Restart] Iris Status : %2, Shutter : %3, Gain : %4")
                        .arg(IrisState).arg(gf_CurrentShtter).arg(gf_CurrentGain);
                qDebug() << logstr;
                log->write(logstr,LOG_NOTICE);

            }
            //light check
            lightckeckcount += gi_BrInterval;
            if( lightckeckcount > 5000) //5sec interval
            {
                lightckeckcount=0;
                LightCheck();

                //day check -> target br init
                QString dat = QDateTime::currentDateTime().toString("dd");
                if(m_datecheck != dat )
                {
                    gi_TargetBr = gi_StandardBr;
                    m_datecheck = dat;
                    qDebug() << "AutoIris Target Br Init";
                    log->write(QString("AutoIris Target Br Init(ch:%2) - TargetBr : %1").arg(gi_TargetBr).arg(m_channel),LOG_INFO);
                }

            }
            commonvalues::systemcheck |= commonvalues::sysautoiris;
        }
        catch( ... )
        {
            logstr = QString("--------- Error Autoiris Camera/Light Check(ch%1) -----------").arg(m_channel);
            log->write(logstr,LOG_ERR);            qDebug() << logstr;
        }

        if( gi_BrInterval < 30) gi_BrInterval = 30;
        msleep(gi_BrInterval);


    }

    logstr = QString("autoiris thread(ch%1) end").arg(m_channel);
    log->write(logstr,LOG_ERR); qDebug() << logstr;
}

void autoiris::stop()
{
    brun = false;
}


//?????? ???????????? ?????? ???????????? ????????? ???????????????.
//?????? ????????? ???????????? ????????? ?????? ??? ??????,????????? ??????
//??? ????????? ????????? ?????? ????????? ????????????.
void autoiris::BR_Input(int Brightness)
{
    try
    {
        //?????????????????? ?????? ?????? ??????
        if(!gb_AutoIrisUse)
        {
            return;
        }

        gi_BrSum += Brightness;
        gi_BrGetCount++;

        //????????? ??????
        if (gi_BrGetCount >= gi_BrAvgCount && gi_BrGetCount > 0)
        {
            gi_BrAvg = gi_BrSum / gi_BrGetCount;
            AutoIris_Action(gi_BrAvg); //?????????????????? ??????
            gi_BrSum = 0;
            gi_BrGetCount = 0;
        }
    }
    catch (exception ex)
    {
        qDebug() << Q_FUNC_INFO <<ex.what();
        log->write(QString("Error(ch:%3)[%1] : %2").arg(Q_FUNC_INFO).arg(ex.what()).arg(m_channel),LOG_ERR);

    }
}

//?????????????????? ?????????
void autoiris::AutoIris_Action(int BRavg)
{
    try
    {
        bool Setting = false;

        // max/min gain check
        if (gf_CurrentShtter > gf_MaxShutter)
        {
            gf_CurrentShtter = gf_MaxShutter;
            Setting = true;
        }
        else if (gf_CurrentShtter < gf_MinShutter)
        {
            gf_CurrentShtter = gf_MinShutter;
            Setting = true;
        }

        //max/min gain check
        if (gf_CurrentGain > gf_MaxGain)
        {
            gf_CurrentGain = gf_MaxGain;
            Setting = true;
        }
        else if (gf_CurrentGain < gf_MinGain)
        {
            gf_CurrentGain = gf_MinGain;
            Setting = true;
        }

        if (!Setting)
        {
            if (gi_TargetBr + gi_BrRange < BRavg) // iris close
            {
                IrisState = Close;

                if (gf_CurrentGain > gf_MinGain)
                {
                    gf_CurrentGain = gf_CurrentGain - gf_GainChangeStep;

                    if (gf_CurrentGain < gf_MinGain)
                    {
                        gf_CurrentGain = gf_MinGain;
                    }
                    Setting = true;
                }
                else    // gi_CurrentGain <= gi_MinGain
                {
                    if (gf_CurrentShtter != gf_MinShutter)
                    {
                        gf_CurrentShtter = gf_CurrentShtter - gf_ShutterChangeStep;

                        if (gf_CurrentShtter < gf_MinShutter)
                        {
                            gf_CurrentShtter = gf_MinShutter;
                        }
                        Setting = true;
                    }

                }
                qDebug() << QString("AutoIris Close(ch%4) : %1,S%2,G%3").arg(BRavg).arg(gf_CurrentShtter).arg(gf_CurrentGain).arg(m_channel);
                //sdw 151224
                if (gf_CurrentGain == gf_MinGain && gf_CurrentShtter == gf_MinShutter) IrisState = LimitClose;


            }
            else if (gi_TargetBr - gi_BrRange > BRavg) // iris open
            {
                IrisState = Open;
                //Console.WriteLine("AutoIris Open: {0}", BRavg);
                if (gf_CurrentShtter < gf_MaxShutter)
                {
                    gf_CurrentShtter = gf_CurrentShtter + gf_ShutterChangeStep;

                    if (gf_CurrentShtter > gf_MaxShutter)
                    {
                        gf_CurrentShtter = gf_MaxShutter;
                    }
                    Setting = true;
                }
                else    // gi_CurrentShtter >= gi_MaxShutter
                {
                    if (gf_CurrentGain != gf_MaxGain)
                    {
                        gf_CurrentGain = gf_CurrentGain + gf_GainChangeStep;

                        if (gf_CurrentGain > gf_MaxGain)
                        {
                            gf_CurrentGain = gf_MaxGain;
                        }
                        Setting = true;
                    }

                }
                qDebug() << QString("AutoIris Open(ch%4) : %1,S%2,G%3").arg(BRavg).arg(gf_CurrentShtter).arg(gf_CurrentGain).arg(m_channel);

                //sdw 151224
                if (gf_CurrentGain == gf_MaxGain && gf_CurrentShtter == gf_MaxShutter) IrisState = LimitOpen;
            }
            else
            {
                IrisState = Stop;
                qDebug() << QString("AutoIris Stop(ch%2): %1").arg(BRavg).arg(m_channel);
            }

        }        

        if (Setting)
        {
            //????????? ?????????
            BrSetEvent(gf_CurrentShtter, gf_CurrentGain); //, gb_LighOnOff);
            log->write(QString("Br(ch:%5) : %1, Iris Status : %2, Shutter : %3, Gain : %4")
                       .arg(BRavg).arg(IrisState).arg(gf_CurrentShtter).arg(gf_CurrentGain).arg(m_channel),LOG_NOTICE);
        }
    }
    catch (exception ex)
    {
        qDebug() << Q_FUNC_INFO <<ex.what();
        log->write(QString("Error(ch:%3)[%1] : %2")
                   .arg(Q_FUNC_INFO).arg(ex.what()).arg(m_channel),LOG_ERR);
    }

}

bool autoiris::LightCheck()
{
    bool lightonoff;
    //?????? on/off ?????? ??????
    if (gb_24HLightOn == true ) //?????? on
    {
        lightonoff = true;
    }
    else if(gf_CurrentShtter >= gf_LightShutter && gf_CurrentGain >= gf_LightOnGain)
    {
        lightonoff = true;
    }
    // ?????? on , off gain < current gain  < on gain
    else if(gb_LighOnOff && gf_CurrentShtter >= gf_LightShutter && gf_CurrentGain > gf_LightOffGain)
    {
        lightonoff = true;
    }
    else // ?????? off
    {
        lightonoff = false;
    }

    if( lightonoff != gb_LighOnOff)
    {
        QString logstr = QString("Set Light Status(ch:%2) : %1").arg(lightonoff).arg(m_channel);
        qDebug() << logstr;
        log->write(logstr,LOG_NOTICE);

    }
    //2016/10/19  camera strobe control -> spc strobe control
    //2017/05/19  signal??? ??????
    emit SetLICStrobe(m_channel,lightonoff);
    gb_LighOnOff = lightonoff;
    //current  lightstatus
    commonvalues::currentvalue[m_channel].cur_light = gb_LighOnOff;

    return lightonoff;
}
//void autoiris::SetSpcStrobe(bool lightonoff)
//{
//    QByteArray data;

//    if(lightonoff) data.append((char)0x01);
//    else data.append((char)0x00);


//    if(pspcdlg != NULL)pspcdlg->SendCmdProtocol(spccommunication::PTC_CB_TRIGGER,spccommunication::PTC_CB_TRIGGER_XN
//                             ,spccommunication::PTL_UNSIGNED_CHAR,0x01,data);
//}

void autoiris::BrSetEvent(float shutter, float gain)// , bool lightonoff)
{
    bool automode = false;
    bool onoff = true;

    camera->SetCameraShutter(shutter);
    camera->SetCameraGain(gain);
    //current shutter/gain monitoring
    commonvalues::currentvalue[m_channel].cur_shutter = shutter;
    commonvalues::currentvalue[m_channel].cur_gain = gain;
}

//?????????/?????? ??????
// ?????? ???????????? ????????? ?????????.
void autoiris::ReflectBackFuc(int PlateBr,IrisActionState irisactionst)
{
    try
    {
        QString strlog;

        //?????????????????? ?????? ?????? ??????
        if (!gb_AutoIrisUse) return;

        //?????? ?????? ?????? ??????
        //?????? ????????? ????????? ??? ??????
        //?????? ????????? ???????????? ??? ?????? ????????? ????????? ?????? ?????? ?????? ??????
        //?????? ????????? ???????????? ??? ?????? ???????????? ????????? ????????? ?????? ?????? ??????
        if (irisactionst != Stop)
        {
            if (irisactionst == LimitOpen)
            {
                //????????? ????????? ??? ?????? ???????????? ?????? ????????? ???????????? ??? ?????? ?????? ?????? ????????? ????????? ?????? ??????.
                // gi_MaxPlateBr?????? ????????? ?????? ????????? ????????? ???????????? ?????? ???.
                if (PlateBr <= gi_MaxPlateBr) return;
            }
            else if (irisactionst == LimitClose)
            {
                //????????? ????????? ??? ?????? ???????????? ?????? ????????? ???????????? ??? ?????? ?????? ?????? ????????? ????????? ?????? ??????.
                // Min_Plate_BR?????? ????????? ?????? ????????? ????????? ???????????? ?????? ???.
                if (PlateBr >= gi_MinPlateBr) return;
            }
            else
            {
                // Open??? Close ????????? ???????????? ??????.
                return;
            }
        }


        // ?????????/?????? ?????? ??????
        //ReflectBackState ReflectBackState;
        if (gi_TargetBr > gi_StandardBr)
        {
            LightState = Backlight; // ??????
        }
        else if (gi_TargetBr < gi_StandardBr)
        {
            LightState = Reflection; // ?????????
        }
        else
        {
            LightState = Normal;
        }

        // ?????? ?????? ??????
        if (PlateBr > gi_MaxPlateBr)
        {
            gi_TargetBr = gi_TargetBr - gi_BrChangeStep;
        }
        else if (PlateBr < gi_MinPlateBr)
        {
            gi_TargetBr = gi_TargetBr + gi_BrChangeStep;
        }
        //??????/?????? ?????? ??????
        if (gi_TargetBr < gi_MinTargetBr) gi_TargetBr = gi_MinTargetBr;
        else if (gi_TargetBr > gi_MaxTargetBr) gi_TargetBr = gi_MaxTargetBr;

        //?????????/?????? ????????? ?????? ?????? ????????? ??????
        switch (LightState)
        {
            case Normal:
                if (!gb_ReflectionMode && gi_TargetBr < gi_StandardBr)  //????????? ?????? disable ????????? TargetBR??? DefaultBR?????? ?????? ??? ??????.
                {
                    gi_TargetBr = gi_StandardBr;
                }
                else if (!gb_BacklightMode && gi_TargetBr > gi_StandardBr) //?????? ?????? disable ????????? TargetBR??? DefaultBR?????? ??? ??? ??????.
                {
                    gi_TargetBr = gi_StandardBr;
                }
                break;
            case Reflection:  //?????????
                if (gi_TargetBr >= gi_StandardBr)
                {
                    gi_TargetBr = gi_StandardBr;   // None
                }
                if(!gb_ReflectionMode)gi_TargetBr = gi_StandardBr;
                break;
            case Backlight:   //??????
                if (gi_TargetBr <= gi_StandardBr)
                {
                    gi_TargetBr = gi_StandardBr;   // None
                }
                if(!gb_BacklightMode)gi_TargetBr = gi_StandardBr;
                break;

        }

        strlog = QString("ReflectBackFuc(ch:%5) : PlateBr %1, LightState %2, TargetBr %3, IrisState %4 ").arg(PlateBr).arg(LightState)
                .arg(gi_TargetBr).arg(irisactionst).arg(m_channel);

        qDebug() << Q_FUNC_INFO << strlog;
        log->write(strlog,LOG_NOTICE);
//        if(gb_DebugMode) APSG_Log.Log_Message(strlog);

    }
    catch (exception ex)
    {
        qDebug() << Q_FUNC_INFO <<ex.what();
//        if(gb_DebugMode) APSG_Log.Log_Message(ex.ToString());
        log->write(QString("Error(ch:%3)[%1] : %2")
                   .arg(Q_FUNC_INFO).arg(ex.what()).arg(m_channel),LOG_ERR);
    }


}
//?????????/?????? ??????
//????????? ????????? ???????????? ????????? ????????????.
void autoiris::ReflectBackFuc2(PlateBrArray PlateBr,IrisActionState irisactionst)
{
    try
    {
        QString strlog;

        //?????????????????? ?????? ?????? ??????
        if (!gb_AutoIrisUse) return;

        //?????? ?????? ?????? ??????
        //?????? ????????? ????????? ??? ??????
        //?????? ????????? ???????????? ??? ?????? ????????? ????????? ?????? ?????? ?????? ??????
        //?????? ????????? ???????????? ??? ?????? ???????????? ????????? ????????? ?????? ?????? ??????
        if (IrisState != irisactionst)
        {
            if (IrisState == irisactionst)
            {
                //????????? ????????? ??? ?????? ???????????? ?????? ????????? ???????????? ??? ?????? ?????? ?????? ????????? ????????? ?????? ??????.
                // gi_MaxPlateBr?????? ????????? ?????? ????????? ????????? ???????????? ?????? ???.
                if (PlateBr.HighPlateBr <= gi_MaxPlateBr) return;
            }
            else if (IrisState == irisactionst)
            {
                //????????? ????????? ??? ?????? ???????????? ?????? ????????? ???????????? ??? ?????? ?????? ?????? ????????? ????????? ?????? ??????.
                // Min_Plate_BR?????? ????????? ?????? ????????? ????????? ???????????? ?????? ???.
                if (PlateBr.LowPlateBr >= gi_MinPlateBr) return;
            }
            else
            {
                // Open??? Close ????????? ???????????? ??????.
                return;
            }
        }


        // ?????????/?????? ?????? ??????
        //ReflectBackState ReflectBackState;
        if (gi_TargetBr > gi_StandardBr)
        {
            LightState = Backlight; // ??????
        }
        else if (gi_TargetBr < gi_StandardBr)
        {
            LightState = Reflection; // ?????????
        }
        else
        {
            LightState = Normal;
        }

        // ?????? ?????? ??????
        if (PlateBr.HighPlateBr > gi_MaxPlateBr)
        {
            gi_TargetBr = gi_TargetBr - gi_BrChangeStep;
        }
        else if (PlateBr.LowPlateBr < gi_MinPlateBr)
        {
            gi_TargetBr = gi_TargetBr + gi_BrChangeStep;
        }
        //??????/?????? ?????? ??????
        if (gi_TargetBr < gi_MinTargetBr) gi_TargetBr = gi_MinTargetBr;
        else if (gi_TargetBr > gi_MaxTargetBr) gi_TargetBr = gi_MaxTargetBr;

        //?????????/?????? ????????? ?????? ?????? ????????? ??????
        switch (LightState)
        {
            case Normal:
                if (!gb_ReflectionMode && gi_TargetBr < gi_StandardBr)  //????????? ?????? disable ????????? TargetBR??? DefaultBR?????? ?????? ??? ??????.
                {
                    gi_TargetBr = gi_StandardBr;
                }
                else if (!gb_BacklightMode && gi_TargetBr > gi_StandardBr) //?????? ?????? disable ????????? TargetBR??? DefaultBR?????? ??? ??? ??????.
                {
                    gi_TargetBr = gi_StandardBr;
                }
                break;
            case Reflection:  //?????????
                if (gi_TargetBr >= gi_StandardBr)
                {
                    gi_TargetBr = gi_StandardBr;   // None
                }
                if( !gb_ReflectionMode )  gi_TargetBr = gi_StandardBr;
                break;
            case Backlight:   //??????
                if (gi_TargetBr <= gi_StandardBr)
                {
                    gi_TargetBr = gi_StandardBr;   // None
                }
                if( !gb_BacklightMode )  gi_TargetBr = gi_StandardBr;
                break;

        }

        strlog = QString("ReflectBackFuc2(ch:%4) : LightState %1, TargetBr %2, IrisState %3")
                .arg(LightState).arg(gi_TargetBr).arg(irisactionst).arg(m_channel);
        qDebug() << Q_FUNC_INFO << strlog;
        log->write(strlog);
//        if (gb_DebugMode) APSG_Log.Log_Message(strlog);

    }
    catch (exception ex)
    {
        qDebug() << Q_FUNC_INFO <<ex.what();
//        if (gb_DebugMode) APSG_Log.Log_Message(ex.ToString());
        log->write(QString("Error[%1] : %2")
                   .arg(Q_FUNC_INFO).arg(ex.what()),LOG_ERR);
    }

}
void autoiris::PlateImage_Input(QByteArray PlateImage, int iPlateWidth, int iPlateHeight,IrisActionState irisactionst)  // R,G,B
{
    int iPlateBr;
    QString strlog;
    //sdw 2016/09/12 NULL -> size()
    if (PlateImage.size() > 0) // != NULL)
    {
        //????????? ?????? ?????? ????????? ?????? ??????
        //?????? ????????? ??????
        iPlateBr = (int)ImagePlateBr(PlateImage, iPlateWidth, iPlateHeight, PlateCalMode);

        if (iPlateBr < 0 )
        {
            strlog = QString("PlateBr(ch:%1) is %2").arg(m_channel).arg(QString::number(iPlateBr));
            qDebug() << Q_FUNC_INFO << strlog;
            log->write(strlog,LOG_NOTICE);
//            if(gb_DebugMode) APSG_Log.Log_Message(strlog);
            return;
        }

        gi_CurrentPlateBr = iPlateBr;

        //????????? ?????? ??????
        if (PlateCalMode == CroppedPlateHighLowBr)
        {
            ReflectBackFuc2(g_PlateBrArray,irisactionst);
        }
        else
        {
            ReflectBackFuc(iPlateBr,irisactionst);
        }

    }
    else
    {
        strlog = QString("PlateImage(ch:%1) is null").arg(m_channel);
        qDebug() << Q_FUNC_INFO << strlog;
        log->write(strlog,LOG_NOTICE);
//        if(gb_DebugMode) APSG_Log.Log_Message(strlog);
    }
}

quint32 autoiris::ImagePlateBr(QByteArray PlateImage, int iPlateWidth, int iPlateHeight, PlateCalModeList PlateCalMode1)
{

    try
    {
        QString strlog;
        QByteArray CopyPlateImage;// = new QByteArray(PlateImage);
        quint32 CopyPlateWidth;
        quint32 CopyPlateHeight;

        //????????? ????????? ??????
        switch (PlateCalMode1)
        {
            case PlateAvgBr:
            case HighPlateAvgBr:
            case LowPlateAvgBr:
            {
                CopyPlateWidth = iPlateWidth;
                CopyPlateHeight = iPlateHeight;
                CopyPlateImage.append(PlateImage);
                //Array.Copy(PlateImage, CopyPlateImage, iPlateWidth * iPlateHeight * 3);
                break;
            }
            case CroppedPlateBr:
            case CroppedPlateHighBr:
            case CroppedPlateLowBr:
            //sdw 160218 ??????
            case CroppedPlateHighLowBr:
            {
                CopyPlateWidth = iPlateWidth >> 1;
                CopyPlateHeight = iPlateHeight >> 1;

                int iCopyWidthLength = iPlateWidth * 3;
                int iCopyPlateWidthLength = CopyPlateWidth * 3;
                int startpoint = (iPlateWidth >> 2) + (iCopyWidthLength * (iPlateHeight >> 2));
                char *data = PlateImage.data();

                for (int i = 0; i < CopyPlateHeight; i++)  //????????? 1/4??? ??????
                {
                    CopyPlateImage.append(&data[startpoint + (iCopyWidthLength * i)],iCopyPlateWidthLength);
                }
                break;
            }
            default :
                return -1;

        }

        //????????? ????????? ??????
//        quint32 brightness = 0;
//        quint32 bravg1 = 0;
//        quint32 numBytes = 0;
//        quint32 x, y;

//        for (y = 0; y < CopyPlateHeight; y++)
//        {
//            for (x = 0; x < CopyPlateWidth; x++)
//            {
//                numBytes = (y * (CopyPlateWidth * 3)) + (x * 3);
//                                                         /// R                            , G                               , B
//                brightness = (quint32)((76 * (quint32)CopyPlateImage[numBytes] + 150 * (quint32)CopyPlateImage[numBytes + 1] + 29 * (quint32)CopyPlateImage[numBytes + 2]) >> 8);
//                bravg1 += brightness;
//            }
//        }

//        quint32 bravgcount = CopyPlateWidth * CopyPlateHeight;
//        if (bravgcount == 0) bravgcount = 1;
//        quint32 bravg = bravg1 / (quint32)bravgcount;

        int height = iPlateHeight;
        int width = iPlateWidth;
        unsigned char *brimg = (unsigned char *)CopyPlateImage.data();
        quint64 brightness = 0;
        quint64 bravg = 0;
        int numBytes = 0;
        int x, y;
        for (y = 0; y < height; y++)
        {
            for (x = 0; x < width; x++)
            {
                numBytes = ((y * width) + x) * 3;
                //brightness += (int)m_brightbuffer[numBytes];
                brightness += (quint64)((76 * (quint32)brimg[numBytes] + 150 * (quint32)brimg[numBytes + 1] + 29 * (quint32)brimg[numBytes + 2]) >> 8);
            }
        }

        quint64 bravgcount = (quint64)width * (quint64)height;
        if (bravgcount == 0) bravgcount = 1;
        bravg = brightness / bravgcount;


        // ????????? ????????? ??????
        if (PlateCalMode1 == PlateAvgBr || PlateCalMode1 == CroppedPlateBr)
        {
            strlog = QString("ImagePlateBr(ch:%5) : PlateImage Avg %1, W%2, H%3, PlateCalMode %4")
                    .arg(bravg).arg(CopyPlateWidth).arg(CopyPlateHeight).arg(PlateCalMode1).arg(m_channel);
            qDebug() << Q_FUNC_INFO << strlog;
            log->write(strlog,LOG_NOTICE);
            //if(gb_DebugMode) APSG_Log.Log_Message(strlog);
            return bravg;
        }

        // ????????? ?????? ?????? ????????? ??????
        quint32 bravghigh = 0;
        quint32 bravghighcount = 0;
        quint32 bravglow = 0;
        quint32 bravglowcount = 0;
        numBytes = 0;
        for (y = 0; y < CopyPlateHeight; y++)
        {
            for (x = 0; x < CopyPlateWidth; x++)
            {
                numBytes = (y * (CopyPlateWidth * 3)) + (x * 3);

                /// R         , G            , B
                brightness = (quint64)((76 * (quint32)brimg[numBytes] + 150 * (quint32)brimg[numBytes + 1] + 29 * (quint32)brimg[numBytes + 2]) >> 8);

                //sdw ?????? ?????? ????????? ??????
                if (brightness > bravg)
                {
                    bravghigh += brightness;
                    bravghighcount++;
                }
                else
                {
                    bravglow += brightness;
                    bravglowcount++;
                }

            }
        }

        if (bravghighcount == 0) bravghighcount = 1;
        bravghigh = bravghigh / bravghighcount;//sdw ??????????????? ???????????? ??????

        if (bravglowcount == 0) bravglowcount = 1;
        bravglow = bravglow / bravglowcount;//sdw ??????????????? ???????????? ??????

        strlog = QString("ImagePlateBr(ch:%7) : PlateImage Avg %1 , High %2 , Low %3, W%4, H%5, PlateCalMode %6").arg(bravg).arg(bravghigh)
                .arg(bravglow).arg(CopyPlateWidth).arg(CopyPlateHeight).arg(PlateCalMode1).arg(m_channel);
        qDebug() << Q_FUNC_INFO << strlog;
        log->write(strlog,LOG_NOTICE);
//        if(gb_DebugMode) APSG_Log.Log_Message(strlog);

        if (PlateCalMode1 == HighPlateAvgBr || PlateCalMode1 == CroppedPlateHighBr)
        {
            return bravghigh;
        }
        else if (PlateCalMode1 == LowPlateAvgBr || PlateCalMode1 == CroppedPlateLowBr)
        {
            return bravglow;
        }
        //sdw 160218 ??????
        else if (PlateCalMode1 == CroppedPlateHighLowBr)
        {
            g_PlateBrArray.AvgPlateBr = bravg;
            g_PlateBrArray.HighPlateBr = bravghigh;
            g_PlateBrArray.LowPlateBr = bravglow;
            return bravg;
        }


        strlog = QString("ImagePlateBr(ch:%2) : PlateCalMode Error - %1").arg(PlateCalMode1).arg(m_channel);
        qDebug() << Q_FUNC_INFO << strlog;
        log->write(strlog,LOG_ERR);
        return -1;  //?????? ????????? ?????????.
    }
    catch (exception ex)
    {
        qDebug() << Q_FUNC_INFO <<ex.what();
        log->write(QString("Error(ch:%3)[%1] : %2")
                   .arg(Q_FUNC_INFO).arg(ex.what()).arg(m_channel),LOG_ERR);
        return -1;
    }
}

//????????? ??????
void autoiris::VehicleImage_Input(QByteArray VehicleImage, int iPlateWidth, int iPlateHeight,IrisActionState irisactionst)  // R,G,B
{
    int iVehicleBr;
    QString strlog;
    //sdw 2016/09/12 NULL -> size(
    if (VehicleImage.size() > 0)// != NULL)
    {
        //????????? ?????? ?????? ????????? ?????? ??????
        //?????? ????????? ??????
        iVehicleBr = (int)ImageVehicleBr(VehicleImage, iPlateWidth, iPlateHeight,irisactionst);

        if (iVehicleBr < 0)
        {
            strlog = QString("iVehicleBr(ch:%2) is %1").arg(iVehicleBr).arg(m_channel);
            qDebug() << Q_FUNC_INFO << strlog;
            log->write(strlog,LOG_NOTICE);
            return;
        }

    }
    else
    {
        strlog = "VehicleImage is null";
        qDebug() << Q_FUNC_INFO << strlog;
    }

}
//????????? ??? ?????? ?????? ??????
quint32 autoiris::ImageVehicleBr(QByteArray VehicleImage, int iPlateWidth, int iPlateHeight,IrisActionState irisactionst)
{

    try
    {
        QString strlog;
        QByteArray CopyPlateImage;
        CopyPlateImage.append(VehicleImage);

//        quint32 CopyPlateWidth = iPlateWidth;
//        quint32 CopyPlateHeight = iPlateHeight;

        //????????? ????????? ??????
//        quint32 brightness = 0;
//        quint32 bravg = 0;
//        quint32 numBytes = 0;
//        quint32 x, y;

//        for (y = 0; y < CopyPlateHeight; y++)
//        {
//            for (x = 0; x < CopyPlateWidth; x++)
//            {
//                numBytes = (y * (CopyPlateWidth * 3)) + (x * 3);
//                /// R                            , G                               , B
//                brightness = (quint32)((76 * (quint32)CopyPlateImage[numBytes] + 150 * (quint32)CopyPlateImage[numBytes + 1] + 29 * (quint32)CopyPlateImage[numBytes + 2]) >> 8);
//                bravg += brightness;
//            }
//        }

//        quint32 bravgcount = CopyPlateWidth * CopyPlateHeight;
//        if (bravgcount == 0) bravgcount = 1;
//        bravg = bravg / bravgcount;


        int height = iPlateWidth;
        int width = iPlateHeight;
        unsigned char *brimg = (unsigned char *)CopyPlateImage.data();
        quint64 brightness = 0;
        quint64 bravg = 0;
        int numBytes = 0;
        int x, y;
        for (y = 0; y < height; y++)
        {
            for (x = 0; x < width; x++)
            {
                numBytes = ((y * width) + x) * 3;
                //brightness += (int)m_brightbuffer[numBytes];
                brightness += (quint64)((76 * (quint32)brimg[numBytes] + 150 * (quint32)brimg[numBytes + 1] + 29 * (quint32)brimg[numBytes + 2]) >> 8);
            }
        }

        quint64 bravgcount = (quint64)width * (quint64)height;
        if (bravgcount == 0) bravgcount = 1;
        bravg = brightness / bravgcount;



         // ????????? ?????? ?????? ????????? ??????
        quint32 bravghigh = 0;
        quint32 bravghighcount = 0;
        quint32 bravglow = 0;
        quint32 bravglowcount = 0;
        numBytes = 0;
        for (y = 0; y < height; y++)
        {
            for (x = 0; x < width; x++)
            {
                numBytes = (y * (width * 3)) + (x * 3);

                /// R         , G            , B
                brightness = (quint64)((76 * (int)brimg[numBytes] + 150 * (quint32)brimg[numBytes + 1] + 29 * (quint32)brimg[numBytes + 2]) >> 8);

                //sdw ?????? ?????? ????????? ??????
                if (brightness > bravg)
                {
                    bravghigh += brightness;
                    bravghighcount++;
                }
                else
                {
                    bravglow += brightness;
                    bravglowcount++;
                }

            }
        }

        if (bravghighcount == 0) bravghighcount = 1;
        bravghigh = bravghigh / bravghighcount;//sdw ??????????????? ???????????? ??????

        if (bravglowcount == 0) bravglowcount = 1;
        bravglow = bravglow / bravglowcount;//sdw ??????????????? ???????????? ??????

        strlog = QString("ImageVehicleBr(ch:%8) : ImageVehicleBr Avg %1 , High %2 , Low %3, W%4, H%5, L%7, PlateCalMode %6")
                .arg(bravg).arg(bravghigh).arg(bravglow).arg(width).arg(height).arg(PlateCalMode).arg(CopyPlateImage.length()).arg(m_channel);
        qDebug() << Q_FUNC_INFO << strlog;
        log->write(strlog,LOG_NOTICE);
//        if (gb_DebugMode) APSG_Log.Log_Message(strlog);

        /////////// ?????? ????????? ??? ????????? ///////////////


         //?????????????????? ?????? ?????? ??????
        if (!gb_NoRecogMode) return bravg;

        //?????? ?????? ?????? ??????
        //?????? ????????? ????????? ??? ??????
        //?????? ????????? ???????????? ??? ?????? ????????? ????????? ?????? ?????? ?????? ??????
        //?????? ????????? ???????????? ??? ?????? ???????????? ????????? ????????? ?????? ?????? ??????
        if (irisactionst != Stop)
        {
            if (irisactionst == LimitOpen)
            {
                //????????? ????????? ??? ?????? ???????????? ?????? ????????? ???????????? ??? ?????? ?????? ?????? ????????? ????????? ?????? ??????.
                // gi_NoRecogMaxBrr?????? ????????? ?????? ????????? ????????? ???????????? ?????? ???.
                if (bravg <= gi_NoRecogMaxBr) return bravg;
            }
            else if (irisactionst == LimitClose)
            {
                //????????? ????????? ??? ?????? ???????????? ?????? ????????? ???????????? ??? ?????? ?????? ?????? ????????? ????????? ?????? ??????.
                //gi_NoRecogMinBr?????? ????????? ?????? ????????? ????????? ???????????? ?????? ???.
                if (bravg >= gi_NoRecogMinBr) return bravg;
            }
            else
            {
                // Open??? Close ????????? ???????????? ??????.
                return bravg;
            }
        }

        // ?????????/?????? ?????? ??????
        if (gi_TargetBr > gi_StandardBr)
        {
            LightState = Backlight; // ??????
        }
        else if (gi_TargetBr < gi_StandardBr)
        {
            LightState = Reflection; // ?????????
        }
        else
        {
            LightState = Normal;
        }

        // ?????? ?????? ??????
        if (bravg > gi_NoRecogMaxBr)
        {
            gi_TargetBr = gi_TargetBr - gi_BrChangeStep;
        }
        else if (bravg < gi_NoRecogMinBr)
        {

            gi_TargetBr = gi_TargetBr + gi_BrChangeStep;
        }
        //??????/?????? ?????? ??????
        if (gi_TargetBr < gi_MinTargetBr) gi_TargetBr = gi_MinTargetBr;
        else if (gi_TargetBr > gi_MaxTargetBr) gi_TargetBr = gi_MaxTargetBr;

        //?????????/?????? ????????? ?????? ?????? ????????? ??????
        switch (LightState)
        {
            case Normal:
                if (!gb_ReflectionMode && gi_TargetBr < gi_StandardBr)  //????????? ?????? disable ????????? TargetBR??? DefaultBR?????? ?????? ??? ??????.
                {
                    gi_TargetBr = gi_StandardBr;
                }
                else if (!gb_BacklightMode && gi_TargetBr > gi_StandardBr) //?????? ?????? disable ????????? TargetBR??? DefaultBR?????? ??? ??? ??????.
                {
                    gi_TargetBr = gi_StandardBr;
                }
            break;
            case Reflection:  //?????????
                if (gi_TargetBr >= gi_StandardBr)
                {
                    gi_TargetBr = gi_StandardBr;   // None
                }
                if(!gb_ReflectionMode)gi_TargetBr = gi_StandardBr;
                break;
            case Backlight:   //??????
                if (gi_TargetBr <= gi_StandardBr)
                {
                    gi_TargetBr = gi_StandardBr;   // None
                }
                if(!gb_BacklightMode)gi_TargetBr = gi_StandardBr;
                break;

        }

        strlog = QString("ImageVehicleBr(ch:%4) : LightState %1, TargetBr %2, IrisState %3").arg(LightState).arg(gi_TargetBr).arg(irisactionst).arg(m_channel);
        qDebug() << Q_FUNC_INFO << strlog;
        log->write(strlog,LOG_NOTICE);

        return bravg;

    }
    catch (exception ex)
    {
        qDebug() << Q_FUNC_INFO <<ex.what();
        log->write(QString("Error(ch:%3)[%1] : %2")
                   .arg(Q_FUNC_INFO).arg(ex.what()).arg(m_channel),LOG_ERR);
        return -1;
    }
}

void autoiris::AutoIrisImageInsert(QByteArray img, int width, int height, int inputtype)
{
    AutoIrisImageStruct aiis;

    aiis.img.append(img);
    aiis.width = width;
    aiis.height = height;
    aiis.inputtype = inputtype;
    aiis.irisst = IrisState;

    queuemutex.lock();
    AutoIrisImg.enqueue(aiis);
    queuemutex.unlock();

}
