#include "controlboarddlg.h"
#include "ui_controlboarddlg.h"
#include "commonvalues.h"

ControlBoardDlg::ControlBoardDlg(int channel, LICComm *liccom, CCUComm *ccucom, TGMCUComm *tgmcu, config *cfg, QWidget *parent) :
    QDialog(parent), ui(new Ui::ControlBoardDlg)
{
    ui->setupUi(this);

    this->setWindowTitle(QString("ControlBoard Ch%1").arg(channel));
    this->setFixedSize(this->geometry().width(),this->geometry().height());

    m_channel = channel;
    pliccomm = liccom;
    pccucomm = ccucom;
    ptgmcu = tgmcu;
    pcfg = cfg;

}

ControlBoardDlg::~ControlBoardDlg()
{
    delete ui;
}

void ControlBoardDlg::showEvent(QShowEvent *e)
{
    ui->leLICPort->setText(commonvalues::cameraSys[m_channel].lic_port);
    ui->leCCUPort->setText(commonvalues::cameraSys[m_channel].ccu_port);
    ui->leTGMCUPort->setText(commonvalues::tgmcu_port);
    ui->cbTGMCUBaudrate->setCurrentText(QString::number(commonvalues::tgmcu_baudrate));

    ui->cbConfirmType->setCurrentIndex(commonvalues::confirmSysType);
    ui->cbCCUType->setCurrentIndex(commonvalues::ccutype);
    ui->chkNewProtocol->setChecked( commonvalues::ccuprotocol == commonvalues::CCUProtocol_New ? true : false);
    ui->chkSeqOneIncrease->setChecked( commonvalues::seqincreaseType == commonvalues::One_Vehicle_One_Seq ? true : false);
    ui->leCCUVioREPTimeout->setText(QString::number(commonvalues::cameraSys[m_channel].ccu_viorepTimout));
    ui->leCCUVioREQDelay->setText(QString::number(commonvalues::ccuViodelay));
    ui->leCCUSendSleep->setText(QString::number(commonvalues::ccuSendsleep));
    ui->chkDayChange->setChecked( commonvalues::daychange > 0 ? true : false );
    ui->chkSyncIgnore->setChecked( commonvalues::syncIgnore > 0 ? true : false );
    ui->chkOBUMatching->setChecked( commonvalues::obuMatch > 0 ? true : false );
    ui->cbOBUMode->setCurrentIndex(commonvalues::obuMode);

    ui->leTriggerREQLane->setText(QString::number(commonvalues::cameraSys[m_channel].TGLane));
    ui->leTriggerLane->setText(QString::number(commonvalues::cameraSys[m_channel].TGLane));
    pliccomm->SettingREQ_Send();
    pliccomm->StatusREQ_Send();
    ptgmcu->SlotLaneREQ_Send();


}

void ControlBoardDlg::ComportReconnection()
{
    if(pliccomm->m_serialport != commonvalues::cameraSys[m_channel].lic_port )
            pliccomm->CloseSerial();
    if(pccucomm->m_serialport != commonvalues::cameraSys[m_channel].ccu_port )
            pccucomm->CloseSerial();
    if(ptgmcu->m_serialport != commonvalues::tgmcu_port)
            ptgmcu->CloseSerial();

    pliccomm->OpenSerial(true,commonvalues::cameraSys[m_channel].lic_port,115200,QSerialPort::Data8
                                         ,QSerialPort::OneStop ,QSerialPort::NoParity,QSerialPort::NoFlowControl);

    pccucomm->OpenSerial(true,commonvalues::cameraSys[m_channel].ccu_port,9600,QSerialPort::Data8
                         ,QSerialPort::OneStop,QSerialPort::NoParity,QSerialPort::NoFlowControl);
    ptgmcu->OpenSerial(true,commonvalues::tgmcu_port,commonvalues::tgmcu_baudrate,QSerialPort::Data8
                       ,QSerialPort::OneStop,QSerialPort::NoParity,QSerialPort::NoFlowControl);
}

void ControlBoardDlg::TextUpdate(int channel,int board, quint8 Cmd, QString data)
{
    if(m_channel != channel) return;

    if(board == boardlic)
    {
        if(Cmd == LICComm::CMD_SettingREQ )
        {
            QString strsetting = data.replace(",","");
            ui->leSettingTrans->setText(strsetting);
        }
        else if(Cmd == LICComm::CMD_StatusREQ)
        {
            QStringList statuslist = data.split(",");
            if( statuslist.count() < 6) return;
            ui->lePresetZoom->setText(statuslist[3]);
            ui->lePresetFocus->setText(statuslist[4]);
            ui->lePresetIris->setText(statuslist[5]);
        }

    }
    else if(board == boardtgmcu)
    {
        if( Cmd = TGMCUComm::CMD_SlotLaneREP)
        {
            QStringList datalist = data.split(",");
            if( datalist.count() < 2) return;
            ui->leSlot->setText(datalist[0]);
            ui->leLane->setText(datalist[1]);
        }
    }
}

void ControlBoardDlg::on_btnComportSave_clicked()
{
    QString licport = ui->leLICPort->text().trimmed();
    QString ccuport = ui->leCCUPort->text().trimmed();
    QString tgmcuport = ui->leTGMCUPort->text().trimmed();
    int tgmcubaudrate = ui->cbTGMCUBaudrate->currentText().trimmed().toInt();

    commonvalues::cameraSys[m_channel].lic_port = licport;
    commonvalues::cameraSys[m_channel].ccu_port = ccuport;
    commonvalues::tgmcu_port = tgmcuport;
    commonvalues::tgmcu_baudrate = tgmcubaudrate;

    QString strchannel = QString("Channel%1").arg(m_channel);
    pcfg->set(QString("%1|LIC").arg(strchannel),"Port",commonvalues::cameraSys[m_channel].lic_port);
    pcfg->set(QString("%1|CCU").arg(strchannel),"Port",commonvalues::cameraSys[m_channel].ccu_port);
    pcfg->set(QString("TGMCU"),"Port",commonvalues::tgmcu_port);
    pcfg->setuint(QString("TGMCU"),"Baudrate",commonvalues::tgmcu_baudrate);

    pcfg->save();

    ComportReconnection();
}

void ControlBoardDlg::on_btnTGCommTest_clicked()
{
    ptgmcu->COMTestREQ_Send();
}


void ControlBoardDlg::on_btnSlotLaneREQ_clicked()
{
    ptgmcu->SlotLaneREQ_Send();
}

void ControlBoardDlg::on_btnTriggerREQ_clicked()
{
    quint8 lane = (quint8)ui->leTriggerREQLane->text().trimmed().toUInt();
    ptgmcu->TriggerREQ_Send(lane);
}

void ControlBoardDlg::on_btnTriggerLaneSave_clicked()
{
    uint lane = (uint)ui->leTriggerLane->text().trimmed().toUInt();
    commonvalues::cameraSys[m_channel].TGLane = lane;

    QString strchannel = QString("Channel%1").arg(m_channel);
    pcfg->setuint(QString("%1|TGMCU").arg(strchannel),"TGLane",commonvalues::cameraSys[m_channel].TGLane);
    pcfg->save();
}

void ControlBoardDlg::on_btnTGMCUReset_clicked()
{
    ptgmcu->ResetREQ_Send();
}

void ControlBoardDlg::on_btnCCUSave_clicked()
{
    int timeout = ui->leCCUVioREPTimeout->text().trimmed().toInt();

    commonvalues::cameraSys[m_channel].ccu_viorepTimout = timeout;
    commonvalues::confirmSysType = ui->cbConfirmType->currentIndex();
    commonvalues::seqincreaseType = ui->chkSeqOneIncrease->isChecked() ? commonvalues::One_Vehicle_One_Seq : commonvalues::One_Vehicle_Two_Seq;
    commonvalues::ccutype = ui->cbCCUType->currentIndex();
    commonvalues::ccuprotocol = ui->chkNewProtocol->isChecked() ? commonvalues::CCUProtocol_New : commonvalues::CCUProtocol_Normal;
    commonvalues::ccuViodelay = ui->leCCUVioREQDelay->text().trimmed().toUInt();
    commonvalues::ccuSendsleep = ui->leCCUSendSleep->text().trimmed().toUInt();
    commonvalues::daychange = ui->chkDayChange->isChecked() ? 1 : 0;
    commonvalues::syncIgnore = ui->chkSyncIgnore->isChecked() ? 1 : 0;
    commonvalues::obuMatch = ui->chkOBUMatching->isChecked() ? 1 : 0;
    commonvalues::obuMode = ui->cbOBUMode->currentIndex();

    QString strchannel = QString("Channel%1").arg(m_channel);
    pcfg->setuint(QString("%1|CCU").arg(strchannel),"VioREPTimeout",commonvalues::cameraSys[m_channel].ccu_viorepTimout);    
    pcfg->setuint(QString("Confirm"),"ConfirmType",commonvalues::confirmSysType);
    pcfg->setuint(QString("CCU"),"seqincreaseType",commonvalues::seqincreaseType);
    pcfg->setuint(QString("CCU"),"CCUType",commonvalues::ccutype);
    pcfg->setuint(QString("CCU"),"CCUProtocolType",commonvalues::ccuprotocol);
    pcfg->setuint(QString("CCU"),"CCUVioDelay",commonvalues::ccuViodelay);
    pcfg->setuint(QString("CCU"),"CCUSendSleep",commonvalues::ccuSendsleep);
    pcfg->setuint(QString("CCU"),"DayChange",commonvalues::daychange);
    pcfg->setuint(QString("CCU"),"SyncIgnore",commonvalues::syncIgnore);
    pcfg->setuint(QString("CCU"),"OBUMatch",commonvalues::obuMatch);
    pcfg->setuint(QString("CCU"),"OBUMode",commonvalues::obuMode);

    pcfg->save();
}

void ControlBoardDlg::on_btnCameraReset_clicked()
{
    pliccomm->Soft_Reset_Send(LICComm::RESET_CAMERA);
}

void ControlBoardDlg::on_btnLICReset_clicked()
{
    pliccomm->Soft_Reset_Send(LICComm::RESET_LIC);
}


void ControlBoardDlg::on_btnLICTrigger_clicked()
{
    emit  SoftTrigger(m_channel);
}

void ControlBoardDlg::on_btnLightOn_clicked()
{
    pliccomm->SetLight(true);
}

void ControlBoardDlg::on_btnLightOff_clicked()
{
    pliccomm->SetLight(false);
}

void ControlBoardDlg::on_btnStatusREQ_clicked()
{
    pliccomm->StatusREQ_Send();
}
void ControlBoardDlg::on_btnPresetControl_clicked()
{
    quint16 ipan = 0;
    quint16 itilt = 0;
    quint16 izoom = (quint16)ui->lePresetZoom->text().trimmed().toUInt();
    quint16 ifocus = (quint16)ui->lePresetFocus->text().trimmed().toUInt();
    quint16 iiris = (quint16)ui->lePresetIris->text().trimmed().toUInt();
    pliccomm->Preset_Control_Send( ipan, itilt, izoom, ifocus, iiris);
}

void ControlBoardDlg::on_btnPresetClose_clicked()
{
    quint16 ipan = 0;
    quint16 itilt = 0;
    quint16 izoom = 0;
    quint16 ifocus = 0;
    quint16 iiris = 1;
    pliccomm->Preset_Control_Send( ipan, itilt, izoom, ifocus, iiris);
}

void ControlBoardDlg::on_btnPresetOpen_clicked()
{
    quint16 ipan = 0;
    quint16 itilt = 0;
    quint16 izoom = 0;
    quint16 ifocus = 0;
    quint16 iiris = 1023;
    pliccomm->Preset_Control_Send( ipan, itilt, izoom, ifocus, iiris);
}

void ControlBoardDlg::on_btnPresetNear_clicked()
{
    quint16 ipan = 0;
    quint16 itilt = 0;
    quint16 izoom = 0;
    quint16 ifocus = 1023;
    quint16 iiris = 0;
    pliccomm->Preset_Control_Send( ipan, itilt, izoom, ifocus, iiris);
}

void ControlBoardDlg::on_btnPresetFar_clicked()
{
    quint16 ipan = 0;
    quint16 itilt = 0;
    quint16 izoom = 0;
    quint16 ifocus = 1;
    quint16 iiris = 0;
    pliccomm->Preset_Control_Send( ipan, itilt, izoom, ifocus, iiris);
}

void ControlBoardDlg::on_btnPresetOut_clicked()
{
    quint16 ipan = 0;
    quint16 itilt = 0;
    quint16 izoom = 1023;
    quint16 ifocus = 0;
    quint16 iiris = 0;
    pliccomm->Preset_Control_Send( ipan, itilt, izoom, ifocus, iiris);
}

void ControlBoardDlg::on_btnPresetIn_clicked()
{
    quint16 ipan = 0;
    quint16 itilt = 0;
    quint16 izoom = 1;
    quint16 ifocus = 0;
    quint16 iiris = 0;
    pliccomm->Preset_Control_Send( ipan, itilt, izoom, ifocus, iiris);
}

void ControlBoardDlg::on_btnTimeClose_clicked()
{
    quint8 type = 4;
    quint8 direction = 1;
    quint16 imove = ui->leTimeIris->text().trimmed().toUInt();
    pliccomm->Time_control_Send(type,direction,imove);
}

void ControlBoardDlg::on_btnTimeOpen_clicked()
{
    quint8 type = 4;
    quint8 direction = 0;
    quint16 imove = ui->leTimeIris->text().trimmed().toUInt();
    pliccomm->Time_control_Send(type,direction,imove);
}

void ControlBoardDlg::on_btnTimeNear_clicked()
{
    quint8 type = 3;
    quint8 direction = 0;
    quint16 imove = ui->leTimeFocus->text().trimmed().toUInt();
    pliccomm->Time_control_Send(type,direction,imove);
}

void ControlBoardDlg::on_btnTimeFar_clicked()
{
    quint8 type = 3;
    quint8 direction = 1;
    quint16 imove = ui->leTimeFocus->text().trimmed().toUInt();
    pliccomm->Time_control_Send(type,direction,imove);
}

void ControlBoardDlg::on_btnTimeOut_clicked()
{
    quint8 type = 2;
    quint8 direction = 1;
    quint16 imove = ui->leTimeZoom->text().trimmed().toUInt();
    pliccomm->Time_control_Send(type,direction,imove);
}

void ControlBoardDlg::on_btnTimeIn_clicked()
{
    quint8 type = 2;
    quint8 direction = 0;
    quint16 imove = ui->leTimeZoom->text().trimmed().toUInt();
    pliccomm->Time_control_Send(type,direction,imove);
}

void ControlBoardDlg::on_btnConfigDataSend_clicked()
{
    QString strvalue = ui->leSettingTrans->text().trimmed();
    bool ok;
    uint hexvalue = strvalue.toUInt(&ok,16);
    if(ok != true) return;
    quint8 bcds = (quint8)(hexvalue & 0x0F);
    quint8 biris = (quint8)(( hexvalue >> 4) & 0x0F);
    quint8 bfocus = (quint8)(( hexvalue >> 8) & 0x0F);
    quint8 bzoom = (quint8)(( hexvalue >> 12) & 0x0F);
    quint8 btilt = (quint8)(( hexvalue >> 16) & 0x0F);
    quint8 bpan = (quint8)(( hexvalue >> 20) & 0x0F);
    quint8 blight = pliccomm->lightINFO.ONOFF;
    pliccomm->SettingTrans_Send(bpan,btilt,bzoom,bfocus,biris,bcds,blight);
}

void ControlBoardDlg::on_btnConfigDataREQ_clicked()
{
    pliccomm->SettingREQ_Send();
}


