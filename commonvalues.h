#ifndef COMMONVALUES_H
#define COMMONVALUES_H

#include <QObject>
#include <QHash>
#include <QDateTime>
#include <QLabel>
#include <QList>

#include "dataclass.h"
#include "spinview.h"
#include "centerclient.h"
#include "ccucomm.h"


class commonvalues : public QObject
{

    //Q_OBJECT   -> no use signal/slot => error
    //struct VehicleData;
public:
    explicit commonvalues( QObject *parent = 0);

#define CAMERASYSTEM_MAXCHANNEL 2
    static int cameraChannel;
    static CameraSystem cameraSys[2];

    /*           TGMCU          */
    static int tgmcu_status;
    static QString tgmcu_port;
    static int tgmcu_baudrate;

    /* Confirm Systme Type */
    enum confirmSysType
    {
        NO_Confirm = 0x00,
        CCU_Two_Confirm = 0x01,
        CCU_One_Confirm = 0x02,
        LPR_Confirm = 0x03
    };
    static int confirmSysType;

    enum SeqType
    {
        One_Vehicle_Two_Seq = 0x00,
        One_Vehicle_One_Seq = 0x01
    };
    static int seqincreaseType;

    enum CCUType
    {
        CCU_NORMAL = 0x00,
        CCU_ITRONICS = 0x01,
        CCU_DAEBO   = 0x02,
        CCU_sTRAFFIC = 0x03,
        CCU_TCS_SDS = 0x04
    };
    static int ccutype;  //itronics : 1  ,nomal : 0

    enum CCUProtocolType
    {
        CCUProtocol_Normal = 0x00,
        CCUProtocol_New = 0x01
    };
    static int ccuprotocol;
    static unsigned int ccuViodelay;
    static unsigned int ccuSendsleep;
    static unsigned int daychange;
    static unsigned int syncIgnore;
    static unsigned int obuMatch;
    enum CCUOBUMode
    {
      CCUOBU_NoReocg = 0x00,
      CCUOBU_OneMismatch = 0x01,
      CCUOBU_TwoMismatch = 0x02,
      CCUOBU_Mismatch = 0x03
    };
    enum OBUMatchType
    {
      OBU_NoMatch = 0x00,
      OBU_Match = 0x01,
      OBU_MisMatch = 0x02
    };
    static unsigned int obuMode;


    enum systemstate
    {
        syscamera = 0x0001,
        sysspc = 0x0002,
        sysrpcu = 0x0004,
        sysrecognition = 0x0008,
        sysmainthread = 0x0010,
        sysautoiris = 0x0020
    };
    static int systemcheck;


    static QList<ViolationInfo> ccuViolationList[2];

 /*        Center  value            */
    static int center_count;
    static QList<CenterInfo> center_list;
    static QList<CenterClient *> clientlist;
    enum FTPRetryType
    {
        FTPRetry_ON = 0x00,
        FTPRetry_OFF = 0x01
    };
    static int ftpretry;

    static uint LaneID; //차로ID
    static uint ManagerID;  //영업소 ID
    static QString LandDirection; //차로방향, I-입구/O-출구/U-상행/D-하행
    static QStringList Check_CarType;
    static QString FTPSavePath;

    static clsRemoteDATA *Remotedata;
    #define LENS_CONTROL_TIME  50
  /*        Database  value            */
    static DataBaseInfo databaseinfo;


   /*        Current value             */
    static CurrentValue currentvalue[2];


    /*    Vehicle Data               */
    static int veicheindex[2];
    static int procvehicleindex[2];

    #define MAX_VEHICLE 10
    #define MAX_VEHICLEINDEX    (MAX_VEHICLE * 10000)  // INDEX   6자릿수  -000000
    static VehicleData *vehicledatalist[2];

    #define MAX_CONFIRM_VEHICLE  5
    static ConfirmData *confirmdatalist;

    enum vehicleprocstate
    {
        spc_tg = 0x0001,
        capture_raw = 0x0002,
        capture_rgb = 0x0004,
        recognition = 0x0008,
        centertrans = 0x0010,
        dbsave = 0x0020
    };

    /*        AutoIris               */

    static AutoIrisInfo autoirisinfo[2];    
    static AutoLightInfo autolightnfo[2];


    static LocalDeviceStatus localdevstatus[2];


   /*        LOG level             */
   static int loglevel;
   static int recogSaveLog;
   //sdw 2017/01/10
   static QHash<QString,QString> lpdatalist;


   static int serial_check_count;

   /*       Firmware Update     */
   enum FIRMWARE_STEP
   {
       FW_NONE = 0x00,
       FW_DOWNLOAD = 0x01,
       FW_UPDATE = 0x02
   };
   static int fw_update;
   static QString fw_controlNo;
   static QString fw_filename;
   static QString fw_resultTime;

};

#endif // COMMONVALUES_H
