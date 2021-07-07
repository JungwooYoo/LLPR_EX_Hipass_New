#ifndef CONTROLBOARDDLG_H
#define CONTROLBOARDDLG_H

#include <QDialog>

#include "liccomm.h"
#include "ccucomm.h"
#include "tgmcucomm.h"
#include "config.h"

namespace Ui {
class ControlBoardDlg;
}

class ControlBoardDlg : public QDialog
{
    Q_OBJECT

public:
    explicit ControlBoardDlg(int channel, LICComm *liccom, CCUComm *ccucom, TGMCUComm *tgmcu, config *cfg ,QWidget *parent = 0);
    ~ControlBoardDlg();
    void showEvent(QShowEvent *e);
    void ComportReconnection();
public slots:
    void TextUpdate(int channel,int board,quint8 Cmd,QString data);
private:
    Ui::ControlBoardDlg *ui;
    int m_channel;
    config *pcfg;
    LICComm *pliccomm;
    CCUComm *pccucomm;
    TGMCUComm *ptgmcu;


public:
    enum boardindex
    {
        boardlic = 0,
        boardccu = 1,
        boardtgmcu = 2,
    };

signals:
    void SoftTrigger(int channel);
private slots:
    void on_btnComportSave_clicked();
    void on_btnTGCommTest_clicked();
    void on_btnSlotLaneREQ_clicked();
    void on_btnTriggerREQ_clicked();
    void on_btnTriggerLaneSave_clicked();
    void on_btnTGMCUReset_clicked();
    void on_btnCCUSave_clicked();
    void on_btnCameraReset_clicked();
    void on_btnLICReset_clicked();
    void on_btnLightOn_clicked();
    void on_btnLightOff_clicked();
    void on_btnPresetControl_clicked();
    void on_btnPresetClose_clicked();
    void on_btnPresetOpen_clicked();
    void on_btnPresetNear_clicked();
    void on_btnPresetFar_clicked();
    void on_btnPresetOut_clicked();
    void on_btnPresetIn_clicked();
    void on_btnTimeClose_clicked();
    void on_btnTimeOpen_clicked();
    void on_btnTimeNear_clicked();
    void on_btnTimeFar_clicked();
    void on_btnTimeOut_clicked();
    void on_btnTimeIn_clicked();
    void on_btnConfigDataSend_clicked();
    void on_btnConfigDataREQ_clicked();

    void on_btnStatusREQ_clicked();
    void on_btnLICTrigger_clicked();
};

#endif // CONTROLBOARDDLG_H
