#include "spinview.h"
#include <QDebug>
#include <QImage>
#include "commonvalues.h"

spinview::spinview(int channel,int loglevel,QThread *parent) : QThread(parent)
{
    brun = false;
    bdisplay = false;
    m_channel = channel;

    log = new Syslogger(this,"camera",true,loglevel);
    m_loglevel = loglevel;

    image_width = 0;
    image_height = 0;

    m_state = STOP;

    //SystemPtr
    system = System::GetInstance();
//    pcamdlg = new cameradlg();
//    pautoiris = new autoiris(this);
//    papsgdlg = new APSGdlg();
}

spinview::~spinview()
{
    log->deleteLater();
}

void spinview::SetLogLevel(int loglevel)
{
    log->m_loglevel = loglevel;
    m_loglevel = loglevel;
}

bool spinview::init()
{

}

bool spinview::start(QString strCamIP ,uint serialNumber)
{
    if(strCamIP.isNull())
        strCamIP = "";

    IPAddress = strCamIP;
    SerialNumber = serialNumber;

    return start();
}

bool spinview::start()
{
    if(isRunning())
    {
        qDebug() << QString("The camera has already running.");
        return true;
    }

    //brun = StartCamera(IPAddress,SerialNumber);
   // if(brun)
        QThread::start();

    return brun;
}

void spinview::run()
{
    QString logstr;
    QImage* displayimg;
    ImagePtr rawImage;
    ImagePtr copyImage = Image::Create();
    int captureindex=0;
    int getimageerror=0;
    QDateTime frametime = QDateTime::currentDateTime();
    //camera grab
    brun = StartCamera(IPAddress,SerialNumber);

    logstr = QString("camera run(ch%1)").arg(m_channel);
    log->write(logstr,LOG_ERR);
    qDebug() << logstr;

    m_state = NORMAL;

    if(brun)
    {
        //sdw 2016/11/17   qlist clear
        if(!procindex.isEmpty())
        {
            procindex.clear();
        }
        emit sigStart(m_channel);
    }

    while(brun)
    {
        try
        {
            rawImage = pcamera->GetNextImage(100);
            getimageerror=0;
        }
        catch( exception ex)
        {
            qDebug() << QString("GetNextImage-%1").arg(ex.what());
            getimageerror++;
            if(getimageerror > 10)
            {
                logstr = QString("--------- Error Camera Thread(ch%1) : GetImage Fail Count Over %2 -----------").arg(m_channel).arg(getimageerror);
                log->write(logstr,LOG_ERR);            qDebug() << logstr;
                brun = false;
            }
            QThread::msleep(10);
            continue;
        }
        if(rawImage == NULL)
        {
            QThread::msleep(10);
            continue;
        }

        if(rawImage->IsIncomplete())
        {
           qDebug() << QString("Image incomplete with image status %1 ...").arg(rawImage->GetImageStatus());
           QThread::msleep(10);
           continue;
        }

        copyImage->DeepCopy(rawImage);
        //copyimage check
        if(copyImage->IsIncomplete())
        {
            qDebug() << QString("Copy Image incomplete with image status %1 ...").arg(rawImage->GetImageStatus());
            continue;
        }

        try
        {
            //recognition
            if(!procindex.isEmpty()) //check capture command
            {
                int count = procindex.size();
                for(int i=0; i < count; i++)
                {
                    int index = procindex.takeFirst();

                    commonvalues::vehicledatalist[m_channel][index].rawImagelen = (unsigned int)copyImage->GetBufferSize();
                    memcpy(commonvalues::vehicledatalist[m_channel][index].rawImage,copyImage->GetData()
                           ,sizeof(unsigned char) * commonvalues::vehicledatalist[m_channel][index].rawImagelen);

                    commonvalues::vehicledatalist[m_channel][index].vehicleproc  |= commonvalues::capture_raw;

                    logstr = QString("ImageCapture Start: index-%1").arg(commonvalues::vehicledatalist[m_channel][index].seq);
                    qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz}") << logstr;
                    log->write(logstr,LOG_NOTICE);
                    if(plogdlg != NULL) plogdlg->logappend(LogDlg::logcamera,QString("Capture %1(%2)")
                                                           .arg(commonvalues::vehicledatalist[m_channel][index].car_num).arg(commonvalues::vehicledatalist[m_channel][index].seq));

                    ImagePtr convertedImage = copyImage->Convert(PixelFormat_RGB8,NEAREST_NEIGHBOR);
                    if(convertedImage == NULL)
                            continue;
                    if(convertedImage->IsIncomplete())
                    {
                        qDebug() << QString("Convert Image incomplete with image status %1 ...").arg(convertedImage->GetImageStatus());
                        continue;
                    }
                    //display image save
                    commonvalues::vehicledatalist[m_channel][index].imglen = convertedImage->GetBufferSize();
                    memcpy(commonvalues::vehicledatalist[m_channel][index].img,convertedImage->GetData(),commonvalues::vehicledatalist[m_channel][index].imglen);
                    commonvalues::vehicledatalist[m_channel][index].vehicleproc  |= commonvalues::capture_rgb;
                    //qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << "ImageCapture - Recog RGB Image event";

                    emit recogcapture(m_channel,index);

                    logstr = QString("ImageCapture End: index-%1").arg(QString::number(commonvalues::vehicledatalist[m_channel][index].seq));
                    qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << logstr;
                    log->write(logstr,LOG_NOTICE);

                    //copyImage->Release();
                }
            }

            //display
            if(bdisplay && captureindex%10 == 0)
            {
                ImagePtr convertedImage = copyImage->Convert(PixelFormat_RGB8,NEAREST_NEIGHBOR);
                if(convertedImage == NULL)
                        continue;
                if(convertedImage->IsIncomplete())
                {
                    qDebug() << QString("display Image incomplete with image status %1 ...").arg(convertedImage->GetImageStatus());
                    continue;
                }

                int width = (int)convertedImage->GetWidth();
                int height = (int)convertedImage->GetHeight();
                int datalen = (int)convertedImage->GetBufferSize();
                displayimg = new QImage( (uchar *)convertedImage->GetData(), width, height, QImage::Format_RGB888);
                emit capture(m_channel,captureindex,displayimg);
            }

            //get bright
            if(captureindex%30 == 0)
            {
                brightmux.lock();
                m_brightrawimglen = (unsigned int)copyImage->GetBufferSize();
                memcpy(m_brightrawimg,copyImage->GetData(),sizeof(unsigned char) * m_brightrawimglen);
                brightmux.unlock();
            }

            rawImage->Release();
            captureindex++;
            if(captureindex%30 == 0)
            {
               qint64 framediff =  frametime.msecsTo(QDateTime::currentDateTime());
               frametime = QDateTime::currentDateTime();
               qint64 framerate = framediff/30;
               commonvalues::currentvalue[m_channel].cur_fps = (int)framerate;
            }
        }
        catch ( ... )
        {
            logstr = QString("--------- Error Camera Thread(ch%1) : Image processing -----------").arg(m_channel);
            log->write(logstr,LOG_ERR);            qDebug() << logstr;
        }


        msleep(1);

    }

    StopCamera();
    emit sigStop(m_channel);
    m_state = STOP;

    logstr = QString("camera stop(ch%1)").arg(m_channel);
    log->write(logstr,LOG_ERR);
    qDebug() << logstr;
}

void spinview::stop()
{
    brun = false;
}

bool spinview::StartCamera(QString strCamIP ,uint serialNumber)
{
    try
    {
        // Retrieve singleton reference to system object


        // Print out current library version
        const LibraryVersion spinnakerLibraryVersion = system->GetLibraryVersion();
        cout << "Spinnaker library version: " << spinnakerLibraryVersion.major << "." << spinnakerLibraryVersion.minor
             << "." << spinnakerLibraryVersion.type << "." << spinnakerLibraryVersion.build << endl
             << endl;

        // Retrieve list of cameras from the system
        camList = system->GetCameras();

        const unsigned int numCameras = camList.GetSize();

        cout << "Number of cameras detected: " << numCameras << endl << endl;

        // Finish if there are no cameras
        if (numCameras == 0)
        {
            // Clear camera list before releasing system
            camList.Clear();

            // Release system
            //system->ReleaseInstance();

            cout << "Not enough cameras!" << endl;
            cout << "Done! Press Enter to exit..." << endl;           

            return false;
        }

        CameraPtr pCam = nullptr;

        int result = 0;

        // Run example on each camera
        if(!strCamIP.isNull() && !strCamIP.isEmpty())
        {
            for (unsigned int i = 0; i < numCameras; i++)
            {
                // Select camera
                pCam = camList.GetByIndex(i);
                int64_t lipaddr = pCam->TLDevice.GevDeviceIPAddress.GetValue();
                QString strip = ConvertIPAddress(lipaddr);
                if(strip.compare(strCamIP) == 0 )
                {
                    pcamera = pCam;
                    break;
                }
            }
        }
        else if( serialNumber != 0 )
        {
            for(unsigned int i =0; i < numCameras; i++)
            {
                pCam = camList.GetByIndex(i);
                QString devicenum = QString(pCam->TLDevice.DeviceSerialNumber.GetValue());
                uint idevicenum = devicenum.toUInt();
                if( idevicenum == serialNumber)
                {
                    pcamera = pCam;
                    break;
                }

            }
        }
        else
            return false;
            //pcamera = camList.GetByIndex(0);

        if( pcamera == NULL )
                return false;

        // Initialize camera
         pcamera->Init();
         int64_t ipaddr = pcamera->GevCurrentIPAddress.GetValue();
         QString strcamipaddr = ConvertIPAddress(ipaddr);
         qDebug() << QString("camera ip : %1").arg(strcamipaddr);

         //if (LogWriteHandler != null) LogWriteHandler(index, string.Format("Ä«¸Þ¶ó ¿¬°á ½Ãµµ : {0}", camera.DeviceSerialNumber.Value));

         uint idevnum = 0;
         QString strdevnum = QString(pcamera->DeviceSerialNumber.GetValue());
         idevnum = strdevnum.toUInt();
         cameraSerialNumber = idevnum;

         if (!SetCameraSetting())
         {
             return false;
         }
#ifdef _DEBUG
        cout << endl << endl << "*** DEBUG ***" << endl << endl;

        // If using a GEV camera and debugging, should disable heartbeat first to prevent further issues
        if (DisableHeartbeat(nodeMap, nodeMapTLDevice) != 0)
        {
            return -1;
        }

        cout << endl << endl << "*** END OF DEBUG ***" << endl << endl;
#endif
         pcamera->BeginAcquisition();

         qDebug() << QString("Acquiring images...");
    }
    catch (Spinnaker::Exception& e)
    {
        //if (LogWriteHandler != null) LogWriteHandler(index,  e.what());
        qDebug() << QString("StartCamera-%1").arg(e.what());
        return false;
    }
    catch (Exception ex)
    {
        //if (LogWriteHandler != null) LogWriteHandler(index, ex.ToString());
        qDebug() << QString("StartCamera-%1").arg(ex.what());
        return false;
    }

    return true;

}

void spinview::StopCamera()
{
    try
    {
        pcamera->EndAcquisition();
        pcamera->DeInit();
//        delete pcamera;
        camList.Clear();
        system->ReleaseInstance();
        //delete system;
    }
    catch (Spinnaker::Exception& e)
    {
        //if (LogWriteHandler != null) LogWriteHandler(index,  e.what());
        qDebug() << QString(e.what());
        return;
    }
    catch (Exception ex)
    {
        //if (LogWriteHandler != null) LogWriteHandler(index, ex.ToString());
        qDebug() << QString(ex.what());
        return;
    }

}

QString spinview::ConvertIPAddress(int64_t ipaddr)
{
    long hex0 = (ipaddr >> 24) & 0xFF;
    long hex1 = (ipaddr >> 16) & 0xFF;
    long hex2 = (ipaddr >> 8) & 0xFF;
    long hex3 = (ipaddr) & 0xFF;
    QString strip = QString("%1.%2.%3.%4").arg(hex0).arg(hex1).arg(hex2).arg(hex3);

    return strip;
}

bool spinview::GetBright(unsigned int *bright)
{
    bool brtn = false;
    //qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << "BR Calculation Enter";

    QImage brightimgcrop;
    int br_sx = commonvalues::cameraSys[m_channel].recog_brroi_sx;
    int br_sy = commonvalues::cameraSys[m_channel].recog_brroi_sy;
    int br_width = commonvalues::cameraSys[m_channel].recog_brroi_width;
    int br_height = commonvalues::cameraSys[m_channel].recog_brroi_height;
    //sdw 2017/05/30
    //Error log 추가
    brightmux.lock();
    try
    {
        ImagePtr rawImage = Image::Create(image_width,image_height,0,0,PixelFormat_BayerRG16,m_brightrawimg);
        ImagePtr convertedImage = rawImage->Convert(PixelFormat_RGB8,NEAREST_NEIGHBOR);
        if(convertedImage == NULL)
                return brtn;
        if(convertedImage->IsIncomplete())
        {
            qDebug() << QString("bright Image incomplete with image status %1 ...").arg(convertedImage->GetImageStatus());
            return brtn;
        }
        QImage displayimg((uchar *)convertedImage->GetData(), image_width, image_height, QImage::Format_RGB888);
        brightimgcrop = displayimg.copy(br_sx,br_sy,br_width,br_height);
    }
    catch(...)
    {
        brightmux.unlock();
        QString logstr = QString("GetBright : Image copy Error");
        qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << logstr;
        log->write(logstr,LOG_ERR);
        return brtn;
    }
    brightmux.unlock();

    //이미지 평균값 계산
    // qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << "BR Calculation MID " << brightimgcrop.format() << brightimg.byteCount();
    int height = brightimgcrop.height();
    int width = brightimgcrop.width();
    unsigned char *brimg = brightimgcrop.bits();
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
            brightness += ((quint64)brimg[numBytes] + (quint64)brimg[numBytes +1]
                           + (quint64)brimg[numBytes + 2] )/3;
        }
    }

    quint64 bravgcount = (quint64)width * (quint64)height;
    if (bravgcount == 0) bravgcount = 1;
    bravg = brightness / bravgcount;

    if(m_loglevel >= LOG_DEBUG)
    {
        QString logstr = QString("GetBright(ch%8) : roi - %1,%2,%3,%4(width:%5,height:%6) , br - %7")
                .arg(br_sx).arg(br_sy).arg(br_width).arg(br_height).arg(width).arg(height).arg(bravg).arg(m_channel);
        qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << logstr;
        log->write(logstr,LOG_DEBUG);
    }

    //qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << "BR Calculation Leave";


    *bright = (unsigned int)bravg;
    brtn = true;
    return brtn;


}

bool spinview::SetCameraSetting()
{
    try
    {
        //---- µ¿¿µ»ó¸ðµå ¼³Á¤
        SetCameraAcquisitionMode();
        //---- shutter, gain auto ²ô±â
        SetCameraAutoExposure();
        SetCameraAutoGain();
        //---- whitebalance auto ²ô±â , red/blue °ª¼³Á¤.
        //---- PixelFormat ¼³Á¤
        SetCameraWhiteBalanceMode("Off");
        QString strPixelFormat = "BayerRG16";
        float fwbRed = 0,fwbBlue = 0;
        //int imgwidth=0, imgheight=0;
        int offsetx=0, offsety=0;
        QString strModel = QString(pcamera->DeviceModelName.GetValue());
        if (strModel.indexOf("BFLY-PGE-20E4C") > -1)
        {
           // cameraValue.KindOfCamera = 1;
            //fwbRed = 1;
            //fwbBlue = 1;
            image_width = 1600;
            image_height = 1200;
        }
        else if (strModel.indexOf("BFLY-PGE-31S4C") > -1)
        {
            //cameraValue.KindOfCamera = 2;
            fwbRed = (float)1.11;
            fwbBlue = (float)2.24;
            offsetx=64;
            offsety=228;
            image_width = 1920;
            image_height = 1080;

        }
        else if (strModel.indexOf("BFLY-PGE-50S5C") > -1)
        {
           // cameraValue.KindOfCamera = 3;
            //fwbRed = 1;
            //fwbBlue = 1;
            strPixelFormat = "BayerRG8";
        }
        SetCameraWhiteBalanceRed(fwbRed);
        SetCameraWhiteBalanceBlue(fwbBlue);

        //---- ¿µ»ó Æ÷¸Ë ¼³Á¤
        //---- µ¿¿µ»ó¸ðµå ½Ã Æ÷¸Ë ¼³Á¤
//        if (cameraValue.ImageExtract || cameraValue.ImageDetection)
//            strPixelFormat = "BayerRG8";
        SetPixelFormatRaw(strPixelFormat);
        //image size
        SetImageSize(image_width, image_height, offsetx, offsety);

//        //---- autoiris ÃÊ±â shutter, gain ¼³Á¤
//        autoirisPsg.gi_CurrentShutter = cameraValue.Shutter;
//        autoirisPsg.gi_CurrentGain = cameraValue.Gain;

        //----- ÃÊ±â shutter, gain°ª ¼³Á¤
        SetCameraShutter(1000);
        SetCameraGain(0);

        ////----
        SetCameraPacketSize_Delay(9000, 3000);
        ////framerate ¼³Á¤
        SetCameraFrameRate(30, false,true);
        //strobe ¼³Á¤
        SetCameraStrobe(true, true, 0, 2.0);

        return true;
    }
    catch (Spinnaker::Exception& e)
    {
        //if (LogWriteHandler != null) LogWriteHandler(index, ex.ToString());
    }
    catch (Exception ex)
    {
        //if (LogWriteHandler != null) LogWriteHandler(index, ex.ToString());
    }

    return false;
}

bool spinview::SetCameraAcquisitionMode(QString strmode)
{
    bool brtn = false;
    try
    {
        INodeMap& nodeMap = pcamera->GetNodeMap();
        // Retrieve enumeration node from nodemap
        CEnumerationPtr ptrAcquisitionMode = nodeMap.GetNode("AcquisitionMode");
        if (!IsAvailable(ptrAcquisitionMode) || !IsWritable(ptrAcquisitionMode))
        {
            cout << "Unable to set acquisition mode to continuous (enum retrieval). Aborting..." << endl << endl;
            return brtn;
        }

        // Retrieve entry node from enumeration node
        CEnumEntryPtr ptrAcquisitionModeContinuous = ptrAcquisitionMode->GetEntryByName(strmode.toUtf8().constData());
        if (!IsAvailable(ptrAcquisitionModeContinuous) || !IsReadable(ptrAcquisitionModeContinuous))
        {
            cout << "Unable to set acquisition mode to continuous (entry retrieval). Aborting..." << endl << endl;
            return brtn;
        }

        // Retrieve integer value from entry node
        const int64_t acquisitionModeContinuous = ptrAcquisitionModeContinuous->GetValue();

        // Set integer value from entry node as new value of enumeration node
        ptrAcquisitionMode->SetIntValue(acquisitionModeContinuous);
        brtn = true;
        qDebug() << QString("Acquisition mode set to %1").arg(strmode);
    }
    catch(exception ex)
    {
        qDebug() << QString("SetCameraAcquisitionMode-%1").arg(ex.what());
        brtn = false;
    }
    return brtn;
}

bool spinview::SetCameraAutoExposure(QString strmode)
{
    bool brtn = false;
    try
    {
        INodeMap& nodeMap = pcamera->GetNodeMap();

        CEnumerationPtr ptrExposureAuto = nodeMap.GetNode("ExposureAuto");
        if (!IsAvailable(ptrExposureAuto) || !IsWritable(ptrExposureAuto))
        {
            qDebug() << QString("Unable to set autoexposure mode to  Aborting...");
            return brtn;
        }


        CEnumEntryPtr ptrExposureAutoOff = ptrExposureAuto->GetEntryByName(strmode.toUtf8().constData());
        if (!IsAvailable(ptrExposureAutoOff) || !IsReadable(ptrExposureAutoOff))
        {
            qDebug() << QString("Unable to set exposureAuto mode to %1. Aborting...").arg(strmode);
            return brtn;
        }

        // Retrieve integer value from entry node
        const int64_t exposureAutoOff = ptrExposureAutoOff->GetValue();

        // Set integer value from entry node as new value of enumeration node
        ptrExposureAuto->SetIntValue(exposureAutoOff);
        brtn = true;
        qDebug() << QString("AutoExposure mode set to %1").arg(strmode);
    }
    catch(exception ex)
    {
        qDebug() << QString("SetCameraAutoExposure-%1").arg(ex.what());
        brtn = false;
    }
    return brtn;
}

bool spinview::SetCameraAutoGain(QString strmode)
{
    bool brtn = false;
    try
    {
        INodeMap& nodeMap = pcamera->GetNodeMap();

        CEnumerationPtr ptrGainAuto = nodeMap.GetNode("GainAuto");
        if (!IsAvailable(ptrGainAuto) || !IsWritable(ptrGainAuto))
        {
            qDebug() << QString("Unable to set GainAuto mode to Aborting...");
            return brtn;
        }


        CEnumEntryPtr ptrGainAutoOff = ptrGainAuto->GetEntryByName(strmode.toUtf8().constData());
        if (!IsAvailable(ptrGainAutoOff) || !IsReadable(ptrGainAutoOff))
        {
            qDebug() << QString("Unable to set GainAuto mode to %1. Aborting...").arg(strmode);
            return brtn;
        }

        // Retrieve integer value from entry node
        const int64_t gainAutoOff = ptrGainAutoOff->GetValue();

        // Set integer value from entry node as new value of enumeration node
        ptrGainAuto->SetIntValue(gainAutoOff);
        brtn = true;
        qDebug() << QString("GainAuto mode set to %1").arg(strmode);
    }
    catch(exception ex)
    {
        qDebug() << QString("GainAuto-%1").arg(ex.what());
        brtn = false;
    }
    return brtn;
}

bool spinview::SetCameraWhiteBalanceMode(QString strmode)
{
    bool brtn = false;
    try
    {
        INodeMap& nodeMap = pcamera->GetNodeMap();

        CEnumerationPtr ptrBalanceWhiteAuto = nodeMap.GetNode("BalanceWhiteAuto");
        if (!IsAvailable(ptrBalanceWhiteAuto) || !IsWritable(ptrBalanceWhiteAuto))
        {
            qDebug() << QString("Unable to set BalanceWhiteAuto mode to Aborting...");
            return brtn;
        }


        CEnumEntryPtr ptrBalanceWhiteAutoOff = ptrBalanceWhiteAuto->GetEntryByName(strmode.toUtf8().constData());
        if (!IsAvailable(ptrBalanceWhiteAutoOff) || !IsReadable(ptrBalanceWhiteAutoOff))
        {
            qDebug() << QString("Unable to set BalanceWhiteAuto mode to %1. Aborting...").arg(strmode);
            return brtn;
        }

        // Retrieve integer value from entry node
        const int64_t balanceWhiteAutoOff = ptrBalanceWhiteAutoOff->GetValue();

        // Set integer value from entry node as new value of enumeration node
        ptrBalanceWhiteAuto->SetIntValue(balanceWhiteAutoOff);
        brtn = true;
        qDebug() << QString("BalanceWhiteAuto mode set to %1").arg(strmode);
    }
    catch(exception ex)
    {
        qDebug() << QString("BalanceWhiteAuto-%1").arg(ex.what());
        brtn = false;
    }
    return brtn;
}

bool spinview::SetCameraWhiteBalanceRed(float fvalue)
{
    bool brtn = false;
    try
    {
        INodeMap& nodeMap = pcamera->GetNodeMap();

        CEnumerationPtr ptrBalanceRatioSelector = nodeMap.GetNode("BalanceRatioSelector");
        if (!IsAvailable(ptrBalanceRatioSelector) || !IsWritable(ptrBalanceRatioSelector))
        {
            qDebug() << QString("Unable to set BalanceRatioSelector mode to Aborting...");
            return brtn;
        }


        CEnumEntryPtr ptrRed = ptrBalanceRatioSelector->GetEntryByName("Red");
        if (!IsAvailable(ptrRed) || !IsReadable(ptrRed))
        {
            qDebug() << QString("Unable to set BalanceRatioSelector mode to Red. Aborting...");
            return brtn;
        }
        const int64_t red = ptrRed->GetValue();
        // Set integer value from entry node as new value of enumeration node
        ptrBalanceRatioSelector->SetIntValue(red);


        CFloatPtr ptrBalanceRatio = nodeMap.GetNode("BalanceRatio");
        if (!IsAvailable(ptrBalanceRatio) || !IsWritable(ptrBalanceRatio))
        {
            qDebug() << QString("Unable to set Red to %1. Aborting...").arg(fvalue);
            return brtn;
        }

        const double balanceRatioMax = ptrBalanceRatio->GetMax();
        double balanceRatioToSet = (double)fvalue;
        if (balanceRatioToSet > balanceRatioMax)
                     balanceRatioToSet = balanceRatioMax;

        ptrBalanceRatio->SetValue(balanceRatioToSet);
        qDebug() << QString("Red set to %1").arg(balanceRatioToSet);
        brtn = true;
    }
    catch(exception ex)
    {
        qDebug() << QString("SetCameraWhiteBalanceRed-%1").arg(ex.what());
        brtn = false;
    }
    return brtn;
}

bool spinview::GetCameraWhiteBalanceRed(float *fvalue)
{
    bool brtn = false;
    try
    {
        INodeMap& nodeMap = pcamera->GetNodeMap();

        CEnumerationPtr ptrBalanceRatioSelector = nodeMap.GetNode("BalanceRatioSelector");
        if (!IsAvailable(ptrBalanceRatioSelector) || !IsWritable(ptrBalanceRatioSelector))
        {
            qDebug() << QString("Unable to set BalanceRatioSelector mode to Aborting...");
            return brtn;
        }


        CEnumEntryPtr ptrRed = ptrBalanceRatioSelector->GetEntryByName("Red");
        if (!IsAvailable(ptrRed) || !IsReadable(ptrRed))
        {
            qDebug() << QString("Unable to set BalanceRatioSelector mode to Red. Aborting...");
            return brtn;
        }
        const int64_t red = ptrRed->GetValue();
        // Set integer value from entry node as new value of enumeration node
        ptrBalanceRatioSelector->SetIntValue(red);


        CFloatPtr ptrBalanceRatio = nodeMap.GetNode("BalanceRatio");
        if (!IsAvailable(ptrBalanceRatio) || !IsWritable(ptrBalanceRatio))
        {
            qDebug() << QString("Unable to get Red. Aborting...");
            return brtn;
        }

        *fvalue = (float)ptrBalanceRatio->GetValue();
        qDebug() << QString("Red get %1").arg(*fvalue);
        brtn = true;
    }
    catch(exception ex)
    {
        qDebug() << QString("GetCameraWhiteBalanceRed-%1").arg(ex.what());
        brtn = false;
    }
    return brtn;
}

bool spinview::SetCameraWhiteBalanceBlue(float fvalue)
{
    bool brtn = false;
    try
    {
        INodeMap& nodeMap = pcamera->GetNodeMap();

        CEnumerationPtr ptrBalanceRatioSelector = nodeMap.GetNode("BalanceRatioSelector");
        if (!IsAvailable(ptrBalanceRatioSelector) || !IsWritable(ptrBalanceRatioSelector))
        {
            qDebug() << QString("Unable to set BalanceRatioSelector mode to Aborting...");
            return brtn;
        }


        CEnumEntryPtr ptrBlue = ptrBalanceRatioSelector->GetEntryByName("Blue");
        if (!IsAvailable(ptrBlue) || !IsReadable(ptrBlue))
        {
            qDebug() << QString("Unable to set BalanceRatioSelector mode to Blue. Aborting...");
            return brtn;
        }
        const int64_t blue = ptrBlue->GetValue();
        // Set integer value from entry node as new value of enumeration node
        ptrBalanceRatioSelector->SetIntValue(blue);


        CFloatPtr ptrBalanceRatio = nodeMap.GetNode("BalanceRatio");
        if (!IsAvailable(ptrBalanceRatio) || !IsWritable(ptrBalanceRatio))
        {
            qDebug() << QString("Unable to set Blue to %1. Aborting...").arg(fvalue);
            return brtn;
        }

        const double balanceRatioMax = ptrBalanceRatio->GetMax();
        double balanceRatioToSet = (double)fvalue;
        if (balanceRatioToSet > balanceRatioMax)
                     balanceRatioToSet = balanceRatioMax;

        ptrBalanceRatio->SetValue(balanceRatioToSet);
        qDebug() << QString("Blue set to %1").arg(balanceRatioToSet);
        brtn = true;
    }
    catch(exception ex)
    {
        qDebug() << QString("SetCameraWhiteBalanceBlue-%1").arg(ex.what());
        brtn = false;
    }
    return brtn;
}

bool spinview::GetCameraWhiteBalanceBlue(float *fvalue)
{
    bool brtn = false;
    try
    {
        INodeMap& nodeMap = pcamera->GetNodeMap();

        CEnumerationPtr ptrBalanceRatioSelector = nodeMap.GetNode("BalanceRatioSelector");
        if (!IsAvailable(ptrBalanceRatioSelector) || !IsWritable(ptrBalanceRatioSelector))
        {
            qDebug() << QString("Unable to set BalanceRatioSelector mode to Aborting...");
            return brtn;
        }


        CEnumEntryPtr ptrBlue = ptrBalanceRatioSelector->GetEntryByName("Blue");
        if (!IsAvailable(ptrBlue) || !IsReadable(ptrBlue))
        {
            qDebug() << QString("Unable to set BalanceRatioSelector mode to Blue. Aborting...");
            return brtn;
        }
        const int64_t blue = ptrBlue->GetValue();
        // Set integer value from entry node as new value of enumeration node
        ptrBalanceRatioSelector->SetIntValue(blue);


        CFloatPtr ptrBalanceRatio = nodeMap.GetNode("BalanceRatio");
        if (!IsAvailable(ptrBalanceRatio) || !IsWritable(ptrBalanceRatio))
        {
            qDebug() << QString("Unable to get Blue. Aborting...");
            return brtn;
        }
        *fvalue = (float)ptrBalanceRatio->GetValue();

        qDebug() << QString("Blue get to %1").arg(*fvalue);
        brtn = true;
    }
    catch(exception ex)
    {
        qDebug() << QString("GetCameraWhiteBalanceBlue-%1").arg(ex.what());
        brtn = false;
    }
    return brtn;
}

bool spinview::SetPixelFormatRaw(QString strmode)
{
    bool brtn = false;
    try
    {
        INodeMap& nodeMap = pcamera->GetNodeMap();

        CEnumerationPtr ptrPixelFormat = nodeMap.GetNode("PixelFormat");
        if (!IsAvailable(ptrPixelFormat) || !IsWritable(ptrPixelFormat))
        {
            qDebug() << QString("Unable to set PixelFormat mode to Aborting...");
            return brtn;
        }


        CEnumEntryPtr ptrPixelFormatRAW = ptrPixelFormat->GetEntryByName(strmode.toUtf8().constData());
        if (!IsAvailable(ptrPixelFormatRAW) || !IsReadable(ptrPixelFormatRAW))
        {
            qDebug() << QString("Unable to set PixelFormatRAW mode to %1. Aborting...").arg(strmode);
            return brtn;
        }

        // Retrieve integer value from entry node
        const int64_t pixelFormatRAW = ptrPixelFormatRAW->GetValue();

        // Set integer value from entry node as new value of enumeration node
        ptrPixelFormat->SetIntValue(pixelFormatRAW);
        brtn = true;
        qDebug() << QString("PixelFormatRAW mode set to %1").arg(strmode);
        brtn = true;
    }
    catch(exception ex)
    {
        qDebug() << QString("SetPixelFormatRaw-%1").arg(ex.what());
        brtn = false;
    }
    return brtn;
}

bool spinview::SetImageSize(int width, int height, int offsetX, int offsetY)
{
    bool brtn = false;
    try
    {
        INodeMap& nodeMap = pcamera->GetNodeMap();

        CIntegerPtr ptrWidth = nodeMap.GetNode("Width");
        if (!IsAvailable(ptrWidth) || !IsWritable(ptrWidth))
        {
            qDebug() << QString("Unable to set Image Width to %1. Aborting...").arg(width);
            return brtn;
        }
        ptrWidth->SetValue((int64_t)width > ptrWidth->GetMax() ? ptrWidth->GetMax() : (int64_t)width);

        CIntegerPtr ptrHeight = nodeMap.GetNode("Height");
        if (!IsAvailable(ptrHeight) || !IsWritable(ptrHeight))
        {
            qDebug() << QString("Unable to set Image Height to %1. Aborting...").arg(height);
            return brtn;
        }
        ptrHeight->SetValue((int64_t)height > ptrHeight->GetMax() ? ptrHeight->GetMax() : (int64_t)height);

        CIntegerPtr ptrOffsetX = nodeMap.GetNode("OffsetX");
        if (!IsAvailable(ptrOffsetX) || !IsWritable(ptrOffsetX))
        {
            qDebug() << QString("Unable to set Image OffsetX to %1. Aborting...").arg(offsetX);
            return brtn;
        }
        ptrOffsetX->SetValue((int64_t)offsetX > ptrOffsetX->GetMax() ? ptrOffsetX->GetMax() : (int64_t)offsetX);

        CIntegerPtr ptrOffsetY = nodeMap.GetNode("OffsetY");
        if (!IsAvailable(ptrOffsetY) || !IsWritable(ptrOffsetY))
        {
            qDebug() << QString("Unable to set Image OffsetY to %1. Aborting...").arg(offsetY);
            return brtn;
        }
        ptrOffsetY->SetValue((int64_t)offsetY > ptrOffsetY->GetMax() ? ptrOffsetY->GetMax() : (int64_t)offsetY);
        brtn = true;

    }
    catch(exception ex)
    {
        qDebug() << QString("SetImageSize-%1").arg(ex.what());
        brtn = false;
    }
    return brtn;
}

bool spinview::GetImageSize(int *width, int *height, int *offgetX, int *offgetY)
{
    bool brtn = false;
    try
    {
        INodeMap& nodeMap = pcamera->GetNodeMap();

        CIntegerPtr ptrWidth = nodeMap.GetNode("Width");
        if (!IsAvailable(ptrWidth) || !IsWritable(ptrWidth))
        {
            qDebug() << QString("Unable to get Image Width. Aborting...");
            return brtn;
        }
        *width = (int)ptrWidth->GetValue();

        CIntegerPtr ptrHeight = nodeMap.GetNode("Height");
        if (!IsAvailable(ptrHeight) || !IsWritable(ptrHeight))
        {
            qDebug() << QString("Unable to get Image Height. Aborting...");
            return brtn;
        }
        *height = (int)ptrHeight->GetValue();

        CIntegerPtr ptrOffsetX = nodeMap.GetNode("OffsetX");
        if (!IsAvailable(ptrOffsetX) || !IsWritable(ptrOffsetX))
        {
            qDebug() << QString("Unable to get Image OffsetX. Aborting...");
            return brtn;
        }
        *offgetX = (int)ptrOffsetX->GetValue();

        CIntegerPtr ptrOffsetY = nodeMap.GetNode("OffsetY");
        if (!IsAvailable(ptrOffsetY) || !IsWritable(ptrOffsetY))
        {
            qDebug() << QString("Unable to get Image OffsetY. Aborting...");
            return brtn;
        }
        *offgetY = (int)ptrOffsetY->GetValue();

        brtn = true;

    }
    catch(exception ex)
    {
        qDebug() << QString("GetImageSize-%1").arg(ex.what());
        brtn = false;
    }
    return brtn;
}

bool spinview::SetCameraPacketSize_Delay(uint packetsize, uint packetdelay)
{
    bool brtn = false;
    try
    {
        INodeMap& nodeMap = pcamera->GetNodeMap();

        CIntegerPtr ptrGevSCPSPacketSize = nodeMap.GetNode("GevSCPSPacketSize");
        if (!IsAvailable(ptrGevSCPSPacketSize) || !IsWritable(ptrGevSCPSPacketSize))
        {
            qDebug() << QString("Unable to set PacketSize to %1. Aborting...").arg(packetsize);
            return brtn;
        }
        ptrGevSCPSPacketSize->SetValue((int64_t)packetsize > ptrGevSCPSPacketSize->GetMax() ? ptrGevSCPSPacketSize->GetMax() : (int64_t)packetsize);

        CIntegerPtr ptrGevSCPD = nodeMap.GetNode("GevSCPD");
        if (!IsAvailable(ptrGevSCPD) || !IsWritable(ptrGevSCPD))
        {
            qDebug() << QString("Unable to set GevSCPD(Packet Delay) to %1. Aborting...").arg(packetdelay);
            return brtn;
        }
        int64_t pdelay = ptrGevSCPD->GetMax();
        pdelay = (int64_t)packetdelay > ptrGevSCPD->GetMax() ? ptrGevSCPD->GetMax() : (int64_t)packetdelay;
        ptrGevSCPD->SetValue(pdelay);
        brtn = true;
    }
    catch(exception ex)
    {
        qDebug() << QString("SetCameraPacketSize_Delay-%1").arg(ex.what());
        brtn = false;
    }
    return brtn;
}

bool spinview::SetCameraShutter(float fvalue)
{
    bool brtn = false;
    try
    {
        INodeMap& nodeMap = pcamera->GetNodeMap();
        CFloatPtr ptrExposureTime = nodeMap.GetNode("ExposureTime");
        if (!IsAvailable(ptrExposureTime) || !IsWritable(ptrExposureTime))
        {
            qDebug() << QString("Unable to set ExposureTime to %1. Aborting...").arg(fvalue);
            return brtn;
        }

        const double exposureTimeMax = ptrExposureTime->GetMax();
        double exposureTimeToSet = (double)fvalue;
        if (exposureTimeToSet > exposureTimeMax)
                     exposureTimeToSet = exposureTimeMax;

        ptrExposureTime->SetValue(exposureTimeToSet);
        brtn = true;
    }
    catch(exception ex)
    {
        qDebug() << QString("SetCameraShutter-%1").arg(ex.what());
        brtn = false;
    }
    return brtn;
}

bool spinview::GetCameraShutter(float *fvalue)
{
    bool brtn = false;
    try
    {
        INodeMap& nodeMap = pcamera->GetNodeMap();
        CFloatPtr ptrExposureTime = nodeMap.GetNode("ExposureTime");
        if (!IsAvailable(ptrExposureTime) || !IsWritable(ptrExposureTime))
        {
            qDebug() << QString("Unable to get ExposureTime. Aborting...");
            return brtn;
        }
        *fvalue = (float)ptrExposureTime->GetValue();

        brtn = true;
    }
    catch(exception ex)
    {
        qDebug() << QString("SetCameraShutter-%1").arg(ex.what());
        brtn = false;
    }
    return brtn;
}

bool spinview::SetCameraGain(float fvalue)
{
    bool brtn = false;
    try
    {
        INodeMap& nodeMap = pcamera->GetNodeMap();
        CFloatPtr ptrGain = nodeMap.GetNode("Gain");
        if (!IsAvailable(ptrGain) || !IsWritable(ptrGain))
        {
            qDebug() << QString("Unable to set Gain to %1. Aborting...").arg(fvalue);
            return brtn;
        }

        const double gainMax = ptrGain->GetMax();
        double gainToSet = (double)fvalue;
        if (gainToSet > gainMax)
                     gainToSet = gainMax;

        ptrGain->SetValue(gainToSet);
        brtn = true;
    }
    catch(exception ex)
    {
        qDebug() << QString("SetCameraGain-%1").arg(ex.what());
        brtn = false;
    }
    return brtn;
}

bool spinview::GetCameraGain(float *fvalue)
{
    bool brtn = false;
    try
    {
        INodeMap& nodeMap = pcamera->GetNodeMap();
        CFloatPtr ptrGain = nodeMap.GetNode("Gain");
        if (!IsAvailable(ptrGain) || !IsWritable(ptrGain))
        {
            qDebug() << QString("Unable to get Gain. Aborting...");
            return brtn;
        }
        *fvalue = (float)ptrGain->GetValue();
        brtn = true;
    }
    catch(exception ex)
    {
        qDebug() << QString("SetCameraGain-%1").arg(ex.what());
        brtn = false;
    }
    return brtn;
}

bool spinview::SetCameraFrameRate(float fvalue, bool bAuto, bool bOnOff)
{
    bool brtn = false;
    try
    {
        INodeMap& nodeMap = pcamera->GetNodeMap();

        CEnumerationPtr ptrAcquisitionFrameRateAuto = nodeMap.GetNode("AcquisitionFrameRateAuto");
        if (!IsAvailable(ptrAcquisitionFrameRateAuto) || !IsWritable(ptrAcquisitionFrameRateAuto))
        {
            qDebug() << QString("Unable to set AcquisitionFrameRateAuto mode to Aborting...");
            return brtn;
        }
        QString strFPSAuto = (bAuto ? "Continuous" : "Off");
        CEnumEntryPtr ptrAcquisitionFrameRateAutoOff = ptrAcquisitionFrameRateAuto->GetEntryByName(strFPSAuto.toUtf8().constData());
        if (!IsAvailable(ptrAcquisitionFrameRateAutoOff) || !IsReadable(ptrAcquisitionFrameRateAutoOff))
        {
            qDebug() << QString("Unable to set AcquisitionFrameRateAuto mode to %1. Aborting...").arg(strFPSAuto);
            return brtn;
        }
        // Retrieve integer value from entry node
        const int64_t acquisitionFrameRateAutoOff = ptrAcquisitionFrameRateAutoOff->GetValue();
        // Set integer value from entry node as new value of enumeration node
        ptrAcquisitionFrameRateAuto->SetIntValue(acquisitionFrameRateAutoOff);
        qDebug() << QString("AcquisitionFrameRateAuto mode set to %1").arg(strFPSAuto);


        CBooleanPtr ptrFrameRateEnable = nodeMap.GetNode("AcquisitionFrameRateEnabled");
        if (!IsAvailable(ptrFrameRateEnable) || !IsWritable(ptrFrameRateEnable))
        {
            qDebug() << QString("Unable to set AcquisitionFrameRateEnabled to %1. Aborting...").arg(bOnOff);
            return brtn;
        }
        ptrFrameRateEnable->SetValue(bOnOff);


        CFloatPtr ptrFrameRate = nodeMap.GetNode("AcquisitionFrameRate");
        if (!IsAvailable(ptrFrameRate) || !IsWritable(ptrFrameRate))
        {
            qDebug() << QString("Unable to set Acquisition Frame Rate to %1. Aborting...").arg(fvalue);
            return brtn;
        }
        const double frameRateMax = ptrFrameRate->GetMax();
        double frameRateToSet = (double)fvalue;
        if (frameRateToSet > frameRateMax)
                     frameRateToSet = frameRateMax;

        ptrFrameRate->SetValue(frameRateToSet);

        brtn = true;
    }
    catch(exception ex)
    {
        qDebug() << QString("SetCameraFrameRate-%1").arg(ex.what());
        brtn = false;
    }
    return brtn;
}

bool spinview::SetCameraStrobe(bool benable, bool bpolarity, float fdelay, float fduration)
{
    bool brtn = false;
    try
    {
        INodeMap& nodeMap = pcamera->GetNodeMap();

        QString strLineName = "Line1";
        CEnumerationPtr ptrLineSelector = nodeMap.GetNode("LineSelector");
        if (!IsAvailable(ptrLineSelector) || !IsWritable(ptrLineSelector))
        {
            qDebug() << QString("Unable to set LineSelector mode to Aborting...");
            return brtn;
        }
        CEnumEntryPtr ptrLineSel1 = ptrLineSelector->GetEntryByName(strLineName.toUtf8().constData());
        if (!IsAvailable(ptrLineSel1) || !IsReadable(ptrLineSel1))
        {
            qDebug() << QString("Unable to set LineSelelct to %1. Aborting...").arg(strLineName);
            return brtn;
        }
        // Retrieve integer value from entry node
        const int64_t lineSel1 = ptrLineSel1->GetValue();
        // Set integer value from entry node as new value of enumeration node
        ptrLineSelector->SetIntValue(lineSel1);
        qDebug() << QString("LineSelect set to %1").arg(strLineName);

        //Light on/off
        QString strLineSource = "";
        if (benable) strLineSource = "ExposureActive";
        else strLineSource = "UserOutput1";
        CEnumerationPtr ptrLineSource = nodeMap.GetNode("LineSource");
        if (!IsAvailable(ptrLineSource) || !IsWritable(ptrLineSource))
        {
            qDebug() << QString("Unable to set LineSource to Aborting...");
            return brtn;
        }
        CEnumEntryPtr ptrSource = ptrLineSource->GetEntryByName(strLineSource.toUtf8().constData());
        if (!IsAvailable(ptrSource) || !IsReadable(ptrSource))
        {
            qDebug() << QString("Unable to set LineSource to %1. Aborting...").arg(strLineSource);
            return brtn;
        }
        // Retrieve integer value from entry node
        const int64_t source = ptrSource->GetValue();
        // Set integer value from entry node as new value of enumeration node
        ptrLineSource->SetIntValue(source);
        qDebug() << QString("LineSource set to %1").arg(strLineSource);


        //polarity
        CBooleanPtr ptrLineInverter = nodeMap.GetNode("LineInverter");
        if (!IsAvailable(ptrLineInverter) || !IsWritable(ptrLineInverter))
        {
            qDebug() << QString("Unable to set LineInverter to %1. Aborting...").arg(benable);
            return brtn;
        }
        ptrLineInverter->SetValue(bpolarity);

        //strobe delay
        CFloatPtr ptrStrobeDelay = nodeMap.GetNode("StrobeDelay");
        if (!IsAvailable(ptrStrobeDelay) || !IsWritable(ptrStrobeDelay))
        {
            qDebug() << QString("Unable to set StrobeDelay to %1. Aborting...").arg(fdelay);
            return brtn;
        }
        const double strobeDelayMax = ptrStrobeDelay->GetMax();
        double strobeDelayToSet = (double)fdelay;
        if (strobeDelayToSet > strobeDelayMax)
                     strobeDelayToSet = strobeDelayMax;

        ptrStrobeDelay->SetValue(strobeDelayToSet);

        //strobe duration
        CFloatPtr ptrStrobeDuration = nodeMap.GetNode("StrobeDuration");
        if (!IsAvailable(ptrStrobeDuration) || !IsWritable(ptrStrobeDuration))
        {
            qDebug() << QString("Unable to set StrobeDuration to %1. Aborting...").arg(fduration);
            return brtn;
        }
        const double strobeDurationMax = ptrStrobeDuration->GetMax();
        double strobeDurationToSet = (double)fduration;
        if (strobeDurationToSet > strobeDurationMax)
                     strobeDurationToSet = strobeDurationMax;

        ptrStrobeDuration->SetValue(strobeDelayToSet);
        brtn = true;
    }
    catch(exception ex)
    {
        qDebug() << QString("SetCameraStrobe-%1").arg(ex.what());
        brtn = false;
    }
    return brtn;
}

bool spinview::GetCameraStrobe(bool *benable, bool *bpolarity, float *fdelay, float *fduration)
{
    bool brtn = false;
    try
    {
        INodeMap& nodeMap = pcamera->GetNodeMap();

        QString strLineName = "Line1";
        CEnumerationPtr ptrLineSelector = nodeMap.GetNode("LineSelector");
        if (!IsAvailable(ptrLineSelector) || !IsWritable(ptrLineSelector))
        {
            qDebug() << QString("Unable to set LineSelector mode to Aborting...");
            return brtn;
        }
        CEnumEntryPtr ptrLineSel1 = ptrLineSelector->GetEntryByName(strLineName.toUtf8().constData());
        if (!IsAvailable(ptrLineSel1) || !IsReadable(ptrLineSel1))
        {
            qDebug() << QString("Unable to set LineSelelct to %1. Aborting...").arg(strLineName);
            return brtn;
        }
        CEnumerationPtr ptrLineSource = nodeMap.GetNode("LineSource");
        if (!IsAvailable(ptrLineSource) || !IsWritable(ptrLineSource))
        {
            qDebug() << QString("Unable to set LineSource to Aborting...");
            return brtn;
        }
        // Retrieve integer value from entry node
        const int64_t lineSel1 = ptrLineSel1->GetValue();
        // Set integer value from entry node as new value of enumeration node
        ptrLineSelector->SetIntValue(lineSel1);
        qDebug() << QString("LineSelect set to %1").arg(strLineName);


        //Light on/off
        const int64_t source = ptrLineSource->GetIntValue();
        CEnumEntryPtr ptrSource = ptrLineSource->GetEntry(source);
        QString strLineSource = QString(ptrSource->GetName().c_str());
        if(strLineSource.contains("ExposureActive"))
            *benable = true;
        else
            *benable = false;

        qDebug() << QString("LineSource get to %1(strobe %2)").arg(strLineSource).arg(*benable ? "ON" : "OFF");


        //polarity
        CBooleanPtr ptrLineInverter = nodeMap.GetNode("LineInverter");
        if (!IsAvailable(ptrLineInverter) || !IsWritable(ptrLineInverter))
        {
            qDebug() << QString("Unable to get LineInverter. Aborting...");
            return brtn;
        }
        *bpolarity  = ptrLineInverter->GetValue();

        //strobe delay
        CFloatPtr ptrStrobeDelay = nodeMap.GetNode("StrobeDelay");
        if (!IsAvailable(ptrStrobeDelay) || !IsWritable(ptrStrobeDelay))
        {
            qDebug() << QString("Unable to get StrobeDelay. Aborting...");
            return brtn;
        }
        *fdelay = (float)ptrStrobeDelay->GetValue();


        //strobe duration
        CFloatPtr ptrStrobeDuration = nodeMap.GetNode("StrobeDuration");
        if (!IsAvailable(ptrStrobeDuration) || !IsWritable(ptrStrobeDuration))
        {
            qDebug() << QString("Unable to get StrobeDuration. Aborting...");
            return brtn;
        }
        *fduration = ptrStrobeDuration->GetValue();

        brtn = true;
    }
    catch(exception ex)
    {
        qDebug() << QString("GetCameraStrobe-%1").arg(ex.what());
        brtn = false;
    }
    return brtn;
}

void spinview::restart(int channel)
{
    if( channel != m_channel) return;

    QString logstr = QString("Camera Restart(ch%1)").arg(m_channel);
    log->write(logstr,LOG_NOTICE);
    if(plogdlg != NULL) plogdlg->logappend(LogDlg::logcamera,logstr);
    stop();
    //close();
    //m_state=NORMAL;
    //bool brtn = init();
    start();
}

