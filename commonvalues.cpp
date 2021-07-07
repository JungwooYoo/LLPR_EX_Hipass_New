#include "commonvalues.h"

commonvalues::commonvalues(QObject *parent) :
    QObject(parent)
{

}

int commonvalues::cameraChannel = 1;
CameraSystem commonvalues::cameraSys[2];

int commonvalues::tgmcu_status;
QString commonvalues::tgmcu_port = "ttyTHS1";
int commonvalues::tgmcu_baudrate = 38400;

int commonvalues::systemcheck = 0;


QList<ViolationInfo> commonvalues::ccuViolationList[2];

int commonvalues::confirmSysType = commonvalues::NO_Confirm;
int commonvalues::seqincreaseType = commonvalues::One_Vehicle_Two_Seq;
int commonvalues::ccutype = 0;
int commonvalues::ccuprotocol = 0;
unsigned int commonvalues::ccuViodelay = 200;
unsigned int commonvalues::ccuSendsleep = 200;
unsigned int commonvalues::daychange = 0;
unsigned int commonvalues::syncIgnore = 0;
unsigned int commonvalues::obuMatch = 0;
unsigned int commonvalues::obuMode = commonvalues::CCUOBU_NoReocg;


/*        Center  value            */
int commonvalues::center_count = 0;
QList<CenterInfo> commonvalues::center_list;
QList<CenterClient *> commonvalues::clientlist;
int commonvalues::ftpretry = 0;

uint commonvalues::LaneID = 0; //차로ID
uint commonvalues::ManagerID = 0;  //영업소 ID
QString commonvalues::LandDirection = "I"; //차로방향, I-입구/O-출구/U-상행/D-하행
QStringList commonvalues::Check_CarType;
QString commonvalues::FTPSavePath = "/media/data/FTP_Trans";

clsRemoteDATA *commonvalues::Remotedata;

/*        Database  value            */
DataBaseInfo commonvalues::databaseinfo;


/*        Current value             */
CurrentValue commonvalues::currentvalue[2];


/*    Vehicle Data               */
int commonvalues::veicheindex[2];
int commonvalues::procvehicleindex[2];

VehicleData* commonvalues::vehicledatalist[2];

ConfirmData* commonvalues::confirmdatalist;

/*        AutoIris               */

AutoIrisInfo commonvalues::autoirisinfo[2];
AutoLightInfo commonvalues:: autolightnfo[2];

LocalDeviceStatus commonvalues::localdevstatus[2];

/*        LOG level             */
int commonvalues::loglevel = LOG_INFO;
int commonvalues::recogSaveLog = 0;

// ETC
QHash<QString,QString> commonvalues::lpdatalist;

int commonvalues::serial_check_count = 5;

/*       Firmware Update     */
int commonvalues::fw_update = 0;
QString commonvalues::fw_controlNo = "";
QString commonvalues::fw_filename = "";
QString commonvalues::fw_resultTime = "";
