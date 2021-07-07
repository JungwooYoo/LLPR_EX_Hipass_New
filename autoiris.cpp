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
            //br  획득
            if(camera->GetBright(&valueA))
            {
                //qDebug() << "BR :" << valueA;
                BR_Input((int)valueA);
                //current br
                commonvalues::currentvalue[m_channel].cur_br = valueA;

            }
            else qDebug() << "Autoiris GetBright Fail";
            //인식 미인식 영상 처리 루틴
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


//현재 밝기값을 받고 밝기값의 개수를 카운팅한다.
//설정 개수가 들어오면 평균값 계산 후 셔터,게인값 제출
//이 함수의 호출이 동작 속도를 좌우한다.
void autoiris::BR_Input(int Brightness)
{
    try
    {
        //오토아이리스 사용 유무 확인
        if(!gb_AutoIrisUse)
        {
            return;
        }

        gi_BrSum += Brightness;
        gi_BrGetCount++;

        //평균값 계산
        if (gi_BrGetCount >= gi_BrAvgCount && gi_BrGetCount > 0)
        {
            gi_BrAvg = gi_BrSum / gi_BrGetCount;
            AutoIris_Action(gi_BrAvg); //오토아이리스 구동
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

//오토아이리스 구동부
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
            //셔터값 적용부
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
    //조명 on/off 여부 확인
    if (gb_24HLightOn == true ) //조명 on
    {
        lightonoff = true;
    }
    else if(gf_CurrentShtter >= gf_LightShutter && gf_CurrentGain >= gf_LightOnGain)
    {
        lightonoff = true;
    }
    // 조명 on , off gain < current gain  < on gain
    else if(gb_LighOnOff && gf_CurrentShtter >= gf_LightShutter && gf_CurrentGain > gf_LightOffGain)
    {
        lightonoff = true;
    }
    else // 조명 off
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
    //2017/05/19  signal로 변경
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

//전반사/역광 처리
// 인식 번호판을 가지고 처리함.
void autoiris::ReflectBackFuc(int PlateBr,IrisActionState irisactionst)
{
    try
    {
        QString strlog;

        //오토아이리스 사용 유무 확인
        if (!gb_AutoIrisUse) return;

        //타겟 밝기 추종 확인
        //타겟 밝기에 추종을 한 경우
        //타겟 밝기에 추종하지 못 하여 밝으나 밝기를 밝게 해야 하는 경우
        //타겟 밝기에 추종하지 못 하여 어두우나 밝기를 어둡게 해야 하는 경우
        if (irisactionst != Stop)
        {
            if (irisactionst == LimitOpen)
            {
                //셔터와 게인을 다 올린 상태에서 타겟 밝기를 추종하지 못 하는 경우 타겟 밝기를 높이는 것을 막음.
                // gi_MaxPlateBr보다 높아서 타겟 밝기를 낮추는 경우에만 진행 됨.
                if (PlateBr <= gi_MaxPlateBr) return;
            }
            else if (irisactionst == LimitClose)
            {
                //셔터와 게인을 다 올린 상태에서 타겟 밝기를 추종하지 못 하는 경우 타겟 밝기를 높이는 것을 막음.
                // Min_Plate_BR보다 낮아서 타멧 밝기를 높이는 경우에만 진행 됨.
                if (PlateBr >= gi_MinPlateBr) return;
            }
            else
            {
                // Open과 Close 시에는 진행하지 않음.
                return;
            }
        }


        // 전반사/역광 상태 확인
        //ReflectBackState ReflectBackState;
        if (gi_TargetBr > gi_StandardBr)
        {
            LightState = Backlight; // 역광
        }
        else if (gi_TargetBr < gi_StandardBr)
        {
            LightState = Reflection; // 전반사
        }
        else
        {
            LightState = Normal;
        }

        // 타겟 밝기 변경
        if (PlateBr > gi_MaxPlateBr)
        {
            gi_TargetBr = gi_TargetBr - gi_BrChangeStep;
        }
        else if (PlateBr < gi_MinPlateBr)
        {
            gi_TargetBr = gi_TargetBr + gi_BrChangeStep;
        }
        //최대/최소 밝기 체크
        if (gi_TargetBr < gi_MinTargetBr) gi_TargetBr = gi_MinTargetBr;
        else if (gi_TargetBr > gi_MaxTargetBr) gi_TargetBr = gi_MaxTargetBr;

        //전반사/역광 모드에 따른 타겟 밝기값 제한
        switch (LightState)
        {
            case Normal:
                if (!gb_ReflectionMode && gi_TargetBr < gi_StandardBr)  //전반사 모드 disable 시에는 TargetBR은 DefaultBR보다 작을 수 없다.
                {
                    gi_TargetBr = gi_StandardBr;
                }
                else if (!gb_BacklightMode && gi_TargetBr > gi_StandardBr) //역광 모드 disable 시에는 TargetBR은 DefaultBR보다 클 수 없다.
                {
                    gi_TargetBr = gi_StandardBr;
                }
                break;
            case Reflection:  //전반사
                if (gi_TargetBr >= gi_StandardBr)
                {
                    gi_TargetBr = gi_StandardBr;   // None
                }
                if(!gb_ReflectionMode)gi_TargetBr = gi_StandardBr;
                break;
            case Backlight:   //역광
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
//전반사/역광 처리
//번호판 밝기값 복수개를 가지고 처리한다.
void autoiris::ReflectBackFuc2(PlateBrArray PlateBr,IrisActionState irisactionst)
{
    try
    {
        QString strlog;

        //오토아이리스 사용 유무 확인
        if (!gb_AutoIrisUse) return;

        //타겟 밝기 추종 확인
        //타겟 밝기에 추종을 한 경우
        //타겟 밝기에 추종하지 못 하여 밝으나 밝기를 밝게 해야 하는 경우
        //타겟 밝기에 추종하지 못 하여 어두우나 밝기를 어둡게 해야 하는 경우
        if (IrisState != irisactionst)
        {
            if (IrisState == irisactionst)
            {
                //셔터와 게인을 다 올린 상태에서 타겟 밝기를 추종하지 못 하는 경우 타겟 밝기를 높이는 것을 막음.
                // gi_MaxPlateBr보다 높아서 타겟 밝기를 낮추는 경우에만 진행 됨.
                if (PlateBr.HighPlateBr <= gi_MaxPlateBr) return;
            }
            else if (IrisState == irisactionst)
            {
                //셔터와 게인을 다 올린 상태에서 타겟 밝기를 추종하지 못 하는 경우 타겟 밝기를 높이는 것을 막음.
                // Min_Plate_BR보다 낮아서 타멧 밝기를 높이는 경우에만 진행 됨.
                if (PlateBr.LowPlateBr >= gi_MinPlateBr) return;
            }
            else
            {
                // Open과 Close 시에는 진행하지 않음.
                return;
            }
        }


        // 전반사/역광 상태 확인
        //ReflectBackState ReflectBackState;
        if (gi_TargetBr > gi_StandardBr)
        {
            LightState = Backlight; // 역광
        }
        else if (gi_TargetBr < gi_StandardBr)
        {
            LightState = Reflection; // 전반사
        }
        else
        {
            LightState = Normal;
        }

        // 타겟 밝기 변경
        if (PlateBr.HighPlateBr > gi_MaxPlateBr)
        {
            gi_TargetBr = gi_TargetBr - gi_BrChangeStep;
        }
        else if (PlateBr.LowPlateBr < gi_MinPlateBr)
        {
            gi_TargetBr = gi_TargetBr + gi_BrChangeStep;
        }
        //최대/최소 밝기 체크
        if (gi_TargetBr < gi_MinTargetBr) gi_TargetBr = gi_MinTargetBr;
        else if (gi_TargetBr > gi_MaxTargetBr) gi_TargetBr = gi_MaxTargetBr;

        //전반사/역광 모드에 따른 타겟 밝기값 제한
        switch (LightState)
        {
            case Normal:
                if (!gb_ReflectionMode && gi_TargetBr < gi_StandardBr)  //전반사 모드 disable 시에는 TargetBR은 DefaultBR보다 작을 수 없다.
                {
                    gi_TargetBr = gi_StandardBr;
                }
                else if (!gb_BacklightMode && gi_TargetBr > gi_StandardBr) //역광 모드 disable 시에는 TargetBR은 DefaultBR보다 클 수 없다.
                {
                    gi_TargetBr = gi_StandardBr;
                }
                break;
            case Reflection:  //전반사
                if (gi_TargetBr >= gi_StandardBr)
                {
                    gi_TargetBr = gi_StandardBr;   // None
                }
                if( !gb_ReflectionMode )  gi_TargetBr = gi_StandardBr;
                break;
            case Backlight:   //역광
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
        //설정에 따른 영상 자르기 또는 복사
        //영상 평균값 계산
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

        //번호판 밝기 추종
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

        //계산할 이미지 선택
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
            //sdw 160218 추가
            case CroppedPlateHighLowBr:
            {
                CopyPlateWidth = iPlateWidth >> 1;
                CopyPlateHeight = iPlateHeight >> 1;

                int iCopyWidthLength = iPlateWidth * 3;
                int iCopyPlateWidthLength = CopyPlateWidth * 3;
                int startpoint = (iPlateWidth >> 2) + (iCopyWidthLength * (iPlateHeight >> 2));
                char *data = PlateImage.data();

                for (int i = 0; i < CopyPlateHeight; i++)  //이미지 1/4로 축소
                {
                    CopyPlateImage.append(&data[startpoint + (iCopyWidthLength * i)],iCopyPlateWidthLength);
                }
                break;
            }
            default :
                return -1;

        }

        //이미지 평균값 계산
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


        // 이미지 평균값 리턴
        if (PlateCalMode1 == PlateAvgBr || PlateCalMode1 == CroppedPlateBr)
        {
            strlog = QString("ImagePlateBr(ch:%5) : PlateImage Avg %1, W%2, H%3, PlateCalMode %4")
                    .arg(bravg).arg(CopyPlateWidth).arg(CopyPlateHeight).arg(PlateCalMode1).arg(m_channel);
            qDebug() << Q_FUNC_INFO << strlog;
            log->write(strlog,LOG_NOTICE);
            //if(gb_DebugMode) APSG_Log.Log_Message(strlog);
            return bravg;
        }

        // 이미지 평균 이상 밝기값 리턴
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

                //sdw 평균 이상 밝기의 평균
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
        bravghigh = bravghigh / bravghighcount;//sdw 평균이상의 밝기값의 평균

        if (bravglowcount == 0) bravglowcount = 1;
        bravglow = bravglow / bravglowcount;//sdw 평균이하의 밝기값의 평균

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
        //sdw 160218 추가
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
        return -1;  //모드 설정이 잘못됨.
    }
    catch (exception ex)
    {
        qDebug() << Q_FUNC_INFO <<ex.what();
        log->write(QString("Error(ch:%3)[%1] : %2")
                   .arg(Q_FUNC_INFO).arg(ex.what()).arg(m_channel),LOG_ERR);
        return -1;
    }
}

//미인식 루틴
void autoiris::VehicleImage_Input(QByteArray VehicleImage, int iPlateWidth, int iPlateHeight,IrisActionState irisactionst)  // R,G,B
{
    int iVehicleBr;
    QString strlog;
    //sdw 2016/09/12 NULL -> size(
    if (VehicleImage.size() > 0)// != NULL)
    {
        //설정에 따른 영상 자르기 또는 복사
        //영상 평균값 계산
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
//미인식 시 전체 영상 계산
quint32 autoiris::ImageVehicleBr(QByteArray VehicleImage, int iPlateWidth, int iPlateHeight,IrisActionState irisactionst)
{

    try
    {
        QString strlog;
        QByteArray CopyPlateImage;
        CopyPlateImage.append(VehicleImage);

//        quint32 CopyPlateWidth = iPlateWidth;
//        quint32 CopyPlateHeight = iPlateHeight;

        //이미지 평균값 계산
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



         // 이미지 평균 이상 밝기값 리턴
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

                //sdw 평균 이상 밝기의 평균
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
        bravghigh = bravghigh / bravghighcount;//sdw 평균이상의 밝기값의 평균

        if (bravglowcount == 0) bravglowcount = 1;
        bravglow = bravglow / bravglowcount;//sdw 평균이하의 밝기값의 평균

        strlog = QString("ImageVehicleBr(ch:%8) : ImageVehicleBr Avg %1 , High %2 , Low %3, W%4, H%5, L%7, PlateCalMode %6")
                .arg(bravg).arg(bravghigh).arg(bravglow).arg(width).arg(height).arg(PlateCalMode).arg(CopyPlateImage.length()).arg(m_channel);
        qDebug() << Q_FUNC_INFO << strlog;
        log->write(strlog,LOG_NOTICE);
//        if (gb_DebugMode) APSG_Log.Log_Message(strlog);

        /////////// 영상 판단부 및 동작부 ///////////////


         //오토아이리스 사용 유무 확인
        if (!gb_NoRecogMode) return bravg;

        //타겟 밝기 추종 확인
        //타겟 밝기에 추종을 한 경우
        //타겟 밝기에 추종하지 못 하여 밝으나 밝기를 밝게 해야 하는 경우
        //타겟 밝기에 추종하지 못 하여 어두우나 밝기를 어둡게 해야 하는 경우
        if (irisactionst != Stop)
        {
            if (irisactionst == LimitOpen)
            {
                //셔터와 게인을 다 올린 상태에서 타겟 밝기를 추종하지 못 하는 경우 타겟 밝기를 높이는 것을 막음.
                // gi_NoRecogMaxBrr보다 높아서 타겟 밝기를 낮추는 경우에만 진행 됨.
                if (bravg <= gi_NoRecogMaxBr) return bravg;
            }
            else if (irisactionst == LimitClose)
            {
                //셔터와 게인을 다 올린 상태에서 타겟 밝기를 추종하지 못 하는 경우 타겟 밝기를 높이는 것을 막음.
                //gi_NoRecogMinBr보다 낮아서 타멧 밝기를 높이는 경우에만 진행 됨.
                if (bravg >= gi_NoRecogMinBr) return bravg;
            }
            else
            {
                // Open과 Close 시에는 진행하지 않음.
                return bravg;
            }
        }

        // 전반사/역광 상태 확인
        if (gi_TargetBr > gi_StandardBr)
        {
            LightState = Backlight; // 역광
        }
        else if (gi_TargetBr < gi_StandardBr)
        {
            LightState = Reflection; // 전반사
        }
        else
        {
            LightState = Normal;
        }

        // 타겟 밝기 변경
        if (bravg > gi_NoRecogMaxBr)
        {
            gi_TargetBr = gi_TargetBr - gi_BrChangeStep;
        }
        else if (bravg < gi_NoRecogMinBr)
        {

            gi_TargetBr = gi_TargetBr + gi_BrChangeStep;
        }
        //최대/최소 밝기 체크
        if (gi_TargetBr < gi_MinTargetBr) gi_TargetBr = gi_MinTargetBr;
        else if (gi_TargetBr > gi_MaxTargetBr) gi_TargetBr = gi_MaxTargetBr;

        //전반사/역광 모드에 따른 타겟 밝기값 제한
        switch (LightState)
        {
            case Normal:
                if (!gb_ReflectionMode && gi_TargetBr < gi_StandardBr)  //전반사 모드 disable 시에는 TargetBR은 DefaultBR보다 작을 수 없다.
                {
                    gi_TargetBr = gi_StandardBr;
                }
                else if (!gb_BacklightMode && gi_TargetBr > gi_StandardBr) //역광 모드 disable 시에는 TargetBR은 DefaultBR보다 클 수 없다.
                {
                    gi_TargetBr = gi_StandardBr;
                }
            break;
            case Reflection:  //전반사
                if (gi_TargetBr >= gi_StandardBr)
                {
                    gi_TargetBr = gi_StandardBr;   // None
                }
                if(!gb_ReflectionMode)gi_TargetBr = gi_StandardBr;
                break;
            case Backlight:   //역광
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
