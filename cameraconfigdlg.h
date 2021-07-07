#ifndef CAMERACONFIGDLG_H
#define CAMERACONFIGDLG_H

#include <QDialog>
#include <QButtonGroup>

#include "spinview.h"
#include "config.h"
#include "autoiris.h"

namespace Ui {
class CameraConfigDlg;
}

class CameraConfigDlg : public QDialog
{
    Q_OBJECT

public:
    explicit CameraConfigDlg(int channel,spinview *pcam, config *pcfg,QWidget *parent = 0);
    ~CameraConfigDlg();

    void showEvent(QShowEvent *e);

private slots:
    void on_btnShutter_clicked();
    void on_btnGain_clicked();
    void on_btnWhiteBalanceRed_clicked();
    void on_btnWhiteBalanceBlue_clicked();
    void on_btnStrobe_clicked();
    void on_btnSave_clicked();
    void on_btnGetsetting_clicked();
    void on_btnCamStart_clicked();
    void on_btnCamStop_clicked();

private:
    Ui::CameraConfigDlg *ui;
    int m_channel;
    spinview *camera;
    config *cfg;

    QButtonGroup *gstrobepolarity;

};

#endif // CAMERACONFIGDLG_H
