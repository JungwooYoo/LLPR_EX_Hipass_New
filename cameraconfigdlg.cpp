#include "cameraconfigdlg.h"
#include "ui_cameraconfigdlg.h"
#include "commonvalues.h"

CameraConfigDlg::CameraConfigDlg(int channel, spinview *pcam, config *pcfg, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CameraConfigDlg)
{
    ui->setupUi(this);

    this->setWindowTitle(QString("Camera Ch%1").arg(channel));
    this->setFixedSize(this->geometry().width(),this->geometry().height());

    m_channel = channel;
    camera = pcam;
    cfg = pcfg;

    gstrobepolarity = new QButtonGroup(this);
    gstrobepolarity->addButton(ui->rbStrobeLow,0);
    gstrobepolarity->addButton(ui->rbStrobeHigh,1);
}

CameraConfigDlg::~CameraConfigDlg()
{
    delete ui;
}

void CameraConfigDlg::showEvent(QShowEvent *e)
{
    ui->leCameraIP->setText(commonvalues::cameraSys[m_channel].cam_ipaddress);
    on_btnGetsetting_clicked();
    QDialog::showEvent(e);
}

void CameraConfigDlg::on_btnShutter_clicked()
{
    float fvalue = ui->leShutter->text().toFloat();
    camera->SetCameraShutter(fvalue);

    commonvalues::currentvalue[m_channel].cur_shutter = fvalue;
}

void CameraConfigDlg::on_btnGain_clicked()
{
    float fvalue = ui->leGain->text().toFloat();
    camera->SetCameraGain(fvalue);

    commonvalues::currentvalue[m_channel].cur_gain = fvalue;
}

void CameraConfigDlg::on_btnWhiteBalanceRed_clicked()
{
    float fvalue = ui->leWBRed->text().toFloat();
    camera->SetCameraWhiteBalanceRed(fvalue);
}

void CameraConfigDlg::on_btnWhiteBalanceBlue_clicked()
{
    float fvalue = ui->leWBBlue->text().toFloat();
    camera->SetCameraWhiteBalanceBlue(fvalue);

}

void CameraConfigDlg::on_btnStrobe_clicked()
{
    int polarity = gstrobepolarity->checkedId();
    camera->SetCameraStrobe(true,polarity == 0 ? false : true,0.0,2.0);
}

void CameraConfigDlg::on_btnSave_clicked()
{
     //commonvalues::CameraSystem camerasys = commonvalues::cameraSys[m_channel];
    QString strtitle = QString("Channel%1|Camera").arg(m_channel);
    cfg->set(strtitle,"IP",ui->leCameraIP->text().trimmed());
    cfg->setfloat(strtitle,"Shutter",ui->leShutter->text().toFloat());
    cfg->setfloat(strtitle,"Gain",ui->leGain->text().toFloat());
    cfg->setfloat(strtitle,"WBRed",ui->leWBRed->text().toFloat());
    cfg->setfloat(strtitle,"WBBlue",ui->leWBBlue->text().toFloat());
    //sdw 2017/02/02
    int polarity = gstrobepolarity->checkedId();
    if(polarity == -1) polarity = 0;
    cfg->setuint(strtitle,"Strobe_Polarity",(unsigned int)polarity);

    cfg->save();
}

void CameraConfigDlg::on_btnGetsetting_clicked()
{
    float fvalue;
    if(camera->GetCameraShutter(&fvalue))
        ui->leShutter->setText(QString::number(fvalue,'f',2));
    else
        ui->leShutter->setText("NG");

    if(camera->GetCameraGain(&fvalue))
        ui->leGain->setText(QString::number(fvalue,'f',2));
    else
        ui->leGain->setText("NG");

    if(camera->GetCameraWhiteBalanceRed(&fvalue))
        ui->leWBRed->setText(QString::number(fvalue,'f',2));
    else
        ui->leWBRed->setText("NG");

    if(camera->GetCameraWhiteBalanceBlue(&fvalue))
        ui->leWBBlue->setText(QString::number(fvalue,'f',2));
    else
        ui->leWBBlue->setText("NG");

    bool benable;
    bool bpolarity;
    float fdelay, fduration;
    if(camera->GetCameraStrobe(&benable,&bpolarity,&fdelay,&fduration))
    {
         QAbstractButton *rbraw = gstrobepolarity->button( bpolarity ? 1 : 0 );
         rbraw->setChecked(true);
    }

}

void CameraConfigDlg::on_btnCamStart_clicked()
{
    QString strIP = ui->leCameraIP->text();
    camera->start(strIP);
}

void CameraConfigDlg::on_btnCamStop_clicked()
{
    camera->stop();
}
