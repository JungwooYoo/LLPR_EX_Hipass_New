#ifndef AUTOIRIS_H
#define AUTOIRIS_H

#include <QThread>
#include <QDateTime>
#include <QMutex>
#include <QQueue>

#include "dataclass.h"
//#include "flycap.h"
#include "spinview.h"
#include "syslogger.h"
//#include "spcdlg.h"

class autoiris : public QThread
{
    Q_OBJECT
    Q_ENUMS(PlateCalModeList)
    Q_ENUMS(ReflectBackState)
    Q_ENUMS(IrisActionState)
public:
    //현재 오토아이리스 상태 : 없음, 멈춤,열기,다열림,닫기, 다닫힘.
     enum IrisActionState
    {
        None,
        Stop,
        Open,
        LimitOpen,
        Close,
        LimitClose
    };
     //전반사/역광 상태
      enum ReflectBackState
     {
         Normal,
         Reflection,
         Backlight
     };
      //번호판 밝기 계산 방식 선택
       enum PlateCalModeList
      {
          PlateAvgBr,
          HighPlateAvgBr,
          LowPlateAvgBr,
          CroppedPlateBr,
          CroppedPlateHighBr,
          CroppedPlateLowBr,
          CroppedPlateHighLowBr
      };
       struct PlateBrArray
       {
           int AvgPlateBr;
           int HighPlateBr;
           int LowPlateBr;
       };
       struct AutoIrisImageStruct{
           QByteArray img;
           int width;
           int height;
           int inputtype;  // 0: plate, 1: vehicle
           IrisActionState irisst;
       };


public:
    explicit autoiris(int channel,spinview *cam,int loglevel = LOG_INFO,QThread *parent = 0);
    ~autoiris();
    void SetLogLevel(int loglevel);
    void init();
    void run();
    void stop();
    //autoiris function
    //현재 밝기값을 받고 밝기값의 개수를 카운팅한다.
    //설정 개수가 들어오면 평균값 계산 후 셔터,게인값 제출
    //이 함수의 호출이 동작 속도를 좌우한다.
    void BR_Input(int Brightness);
    //오토아이리스 구동부
    void AutoIris_Action(int BRavg);
    //전반사/역광 처리
    // 인식 번호판을 가지고 처리함.
    void ReflectBackFuc(int PlateBr,IrisActionState irisactionst);
    //전반사/역광 처리
    //번호판 밝기값 복수개를 가지고 처리한다.
    void ReflectBackFuc2(PlateBrArray PlateBr,IrisActionState irisactionst);
    void PlateImage_Input(QByteArray PlateImage, int iPlateWidth, int iPlateHeight,IrisActionState);
    quint32 ImagePlateBr(QByteArray PlateImage, int iPlateWidth, int iPlateHeight, PlateCalModeList PlateCalMode1);
    //미인식 루틴
    void VehicleImage_Input(QByteArray VehicleImage, int iPlateWidth, int iPlateHeight,IrisActionState irisactionst);
    //미인식 시 전체 영상 계산
    quint32 ImageVehicleBr(QByteArray VehicleImage, int iPlateWidth, int iPlateHeight,IrisActionState irisactionst);
    void AutoIrisImageInsert(QByteArray img, int width, int height, int inputtype);
    void BrSetEvent(float shutter, float gain ); //, bool lightonoff);
    bool LightCheck();
    //void SetSpcStrobe(bool lightonoff);

signals:
    void SetLICStrobe(int channel,bool lightonoff);
public slots:


public:
    bool brun;
    //FlyCap *camera;
    spinview *camera;
    //spcdlg *pspcdlg;
    int m_channel;

    QString m_datecheck;
    Syslogger *log;
    int m_loglevel;

    //autoiris values
    /* 변수 정의 */
       //auto iris 사용유무
    bool gb_AutoIrisUse;
       //기준 밝기
    int gi_StandardBr;
       //추종 밝기 범위
    int gi_BrRange;
       //밝기 계산 개수 - 구동 주기
    int gi_BrAvgCount;
       // ms
    int gi_BrInterval;
       //최대 셔터
    float gf_MaxShutter;
       //최소 셔터
    float gf_MinShutter;
       //셔터 변화 폭
    float gf_ShutterChangeStep;
       //최대 게인
    float gf_MaxGain;
       //최소 게인
    float gf_MinGain;
       //게인 변화 폭
    float gf_GainChangeStep;
       //밝기값 합
    int gi_BrSum;
       //밝기값 획득 수
    int gi_BrGetCount;
       //평균 밝기값
    int gi_BrAvg;
       //현재 타겟 밝기
    int gi_TargetBr;
       //현재 밝기
    int gi_CurrentBr;
       //현재 shutter
    float gf_CurrentShtter;
       //현재 게인
    float gf_CurrentGain;
    //현재 오토아이리스 상태
    IrisActionState IrisState;
       //현재 주/야간 상태

       //현재 번호판 밝기
    int gi_CurrentPlateBr;
       //최대 번호판 밝기
    int gi_MaxPlateBr;
       //최소 번호판 밝기
    int gi_MinPlateBr;
       //최대 추종 밝기
    int gi_MaxTargetBr;
       //최소 추종 밝기
    int gi_MinTargetBr;
       //추종 밝기 변화 폭
    int gi_BrChangeStep;
    //전반사/역광 상태
    ReflectBackState LightState;

       //전반사 모드 사용
    bool gb_ReflectionMode;
       //역광 모드 사용
    bool gb_BacklightMode;
       //밝기값 변경을 위한 밝기 범위 오버 횟수
    int gi_BrChangeCount;
      //번호판 밝기 계산 방식 선택
    PlateCalModeList PlateCalMode;
       //기타 설정
    bool gb_LighOnOff;
    float gf_LightShutter;
    //sdw 2017/01/10 light on/off gap
    float gf_LightOnGain;
    float gf_LightOffGain;
    bool gb_24HLightOn;

    bool gb_DebugMode;

    bool gb_NoRecogMode;
    int gi_NoRecogMaxBr;
    int gi_NoRecogMinBr;

    PlateBrArray g_PlateBrArray;

    QQueue<AutoIrisImageStruct> AutoIrisImg;
    QMutex queuemutex;

    bool m_bcamerarestart;

};

#endif // AUTOIRIS_H
