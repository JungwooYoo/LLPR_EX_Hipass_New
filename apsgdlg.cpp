 #include "apsgdlg.h"
#include "ui_apsgdlg.h"
#include "commonvalues.h"

APSGDlg::APSGDlg(int channel, autoiris *ati, config *pcfg, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::APSGDlg)
{
    ui->setupUi(this);

    this->setWindowTitle(QString("APSG Ch%1").arg(channel));
    this->setFixedSize(this->geometry().width(),this->geometry().height());

    m_channel = channel;
    ai = ati;
    cfg = pcfg;

    QMetaObject MetaObject = ai->staticMetaObject;
    QMetaEnum MetaEnum = MetaObject.enumerator(MetaObject.indexOfEnumerator("PlateCalModeList"));
    int count = MetaEnum.keyCount();
    for( int i=0; i < count; i++)
    {
        ui->cmbAIReflectBackMode->addItem(QString(MetaEnum.valueToKey(i)));
    }
}

APSGDlg::~APSGDlg()
{
    delete ui;
}

void APSGDlg::showEvent(QShowEvent *e)
{
    AutoIrisGetsetting();
    LightGetsetting();

    QDialog::showEvent(e);
}

void APSGDlg::AutoIrisGetsetting()
{
    AutoIrisInfo aiinfo = commonvalues::autoirisinfo[m_channel];

    ui->cbAutoIris->setChecked(aiinfo.autoiris_use);
    //basic
    ui->leAIStandardBR->setText(QString::number(aiinfo.autoiris_standardbr));
    ui->leAIBRRange->setText(QString::number(aiinfo.autoiris_brrange));
    ui->leAIBRInterval->setText(QString::number(aiinfo.autoiris_brinterval));
    ui->leAIBRAvgCount->setText(QString::number(aiinfo.autoiris_bravgcount));
    ui->leAIMaxShutter->setText(QString::number(aiinfo.autoiris_maxshutter,'f',2));
    ui->leAIMinShutter->setText(QString::number(aiinfo.autoiris_minshutter,'f',2));
    ui->leAIShutterStep->setText(QString::number(aiinfo.autoiris_shutterstep,'f',2));
    ui->leAIMaxGain->setText(QString::number(aiinfo.autoiris_maxgain,'f',2));
    ui->leAIMinGain->setText(QString::number(aiinfo.autoiris_mingain,'f',2));
    ui->leAIGainStep->setText(QString::number(aiinfo.autoiris_gainstep,'f',2));
    ui->cbAIDebug->setChecked(aiinfo.autoiris_debug);
    //reflection/back
    ui->cbAIReflection->setChecked(aiinfo.autoiris_reflection);
    ui->cbAIBackLight->setChecked(aiinfo.autoiris_backlight);
    ui->leAIMaxBR->setText(QString::number(aiinfo.autoiris_maxbr));
    ui->leAIMinBR->setText(QString::number(aiinfo.autoiris_minbr));
    ui->leAIBRStep->setText(QString::number(aiinfo.autoiris_brstep));
    ui->leAIMaxPlateBR->setText(QString::number(aiinfo.autoiris_maxplatebr));
    ui->leAIMinPlateBR->setText(QString::number(aiinfo.autoiris_minplatebr));
    ui->leAIBRChangeCount->setText(QString::number(aiinfo.autoiris_brchangecount));
    ui->cmbAIReflectBackMode->setCurrentIndex(aiinfo.autoiris_reflectbackmode);
    //no Recog
    ui->cbAINoRecog->setChecked(aiinfo.autoiris_norecog);
    ui->leAINRMaxBR->setText(QString::number(aiinfo.autoiris_norecogmaxbr));
    ui->leAINRMinBR->setText(QString::number(aiinfo.autoiris_norecogminbr));
}

void APSGDlg::LightGetsetting()
{
    AutoLightInfo alinfo = commonvalues::autolightnfo[m_channel];

    ui->cbLight24H->setChecked(alinfo.light_24h);
    ui->leLightShutter->setText(QString::number(alinfo.light_shutter,'f',2));
    ui->leLightOnGain->setText(QString::number(alinfo.light_ongain,'f',2));
    //sdw 2017/01/10 light on/off gap
    ui->leLightOffGain->setText(QString::number(alinfo.light_offgain,'f',2));

}

void APSGDlg::AutoIrisSave()
{
    AutoIrisInfo aiinfo = commonvalues::autoirisinfo[m_channel];

    QString strtitle = QString("Channel%1|AutoIris|Basic").arg(m_channel);
    cfg->setbool(strtitle,"Use",aiinfo.autoiris_use);
    cfg->setuint(strtitle,"StandardBr",aiinfo.autoiris_standardbr);
    cfg->setuint(strtitle,"BrRange",aiinfo.autoiris_brrange);
    cfg->setuint(strtitle,"BrInterval",aiinfo.autoiris_brinterval);
    cfg->setuint(strtitle,"BrAvgCount",aiinfo.autoiris_bravgcount);
    cfg->setfloat(strtitle,"MaxShutter",aiinfo.autoiris_maxshutter);
    cfg->setfloat(strtitle,"MinShutter",aiinfo.autoiris_minshutter);
    cfg->setfloat(strtitle,"ShutterStep",aiinfo.autoiris_shutterstep);
    cfg->setfloat(strtitle,"MaxGain",aiinfo.autoiris_maxgain);
    cfg->setfloat(strtitle,"MinGain",aiinfo.autoiris_mingain);
    cfg->setfloat(strtitle,"GainStep",aiinfo.autoiris_gainstep);
    cfg->setbool(strtitle,"Debug",aiinfo.autoiris_debug);

    strtitle = QString("Channel%1|AutoIris|Reflection&Back").arg(m_channel);
    cfg->setbool(strtitle,"Reflection",aiinfo.autoiris_reflection);
    cfg->setbool(strtitle,"Backlight",aiinfo.autoiris_backlight);
    cfg->setuint(strtitle,"MaxBr",aiinfo.autoiris_maxbr);
    cfg->setuint(strtitle,"MinBr",aiinfo.autoiris_minbr);
    cfg->setuint(strtitle,"BrStep",aiinfo.autoiris_brstep);
    cfg->setuint(strtitle,"MaxPlateBr",aiinfo.autoiris_maxplatebr);
    cfg->setuint(strtitle,"MinPlateBr",aiinfo.autoiris_minplatebr);
    cfg->setuint(strtitle,"BrChangeCount",aiinfo.autoiris_brchangecount);
    cfg->setuint(strtitle,"Mode",aiinfo.autoiris_reflectbackmode);

    strtitle = QString("Channel%1|AutoIris|NoRecog").arg(m_channel);
    cfg->setbool(strtitle,"Use",aiinfo.autoiris_norecog);
    cfg->setuint(strtitle,"MaxBr",aiinfo.autoiris_norecogmaxbr);
    cfg->setuint(strtitle,"MinBr",aiinfo.autoiris_norecogminbr);

    cfg->save();
}

void APSGDlg::LightSave()
{
    AutoLightInfo alinfo = commonvalues::autolightnfo[m_channel];

    QString strchannel = QString("Channel%1").arg(m_channel);
    QString strtitle = QString("%1|Light").arg(strchannel);
    cfg->setbool(strtitle,"24H",alinfo.light_24h);
    cfg->setfloat(strtitle,"Shutter",alinfo.light_shutter);
    //sdw 2017/01/10
    cfg->setfloat(strtitle,"OnGain",alinfo.light_ongain);
    cfg->setfloat(strtitle,"OffGain",alinfo.light_offgain);

    cfg->save();
}

void APSGDlg::on_btnAISave_clicked()
{
    commonvalues::autoirisinfo[m_channel].autoiris_use = ui->cbAutoIris->isChecked();
    //basic
    commonvalues::autoirisinfo[m_channel].autoiris_standardbr = ui->leAIStandardBR->text().toInt();
    commonvalues::autoirisinfo[m_channel].autoiris_brrange = ui->leAIBRRange->text().toInt();
    commonvalues::autoirisinfo[m_channel].autoiris_brinterval = ui->leAIBRInterval->text().toInt();
    commonvalues::autoirisinfo[m_channel].autoiris_bravgcount = ui->leAIBRAvgCount->text().toInt();
    commonvalues::autoirisinfo[m_channel].autoiris_maxshutter = ui->leAIMaxShutter->text().toFloat();
    commonvalues::autoirisinfo[m_channel].autoiris_minshutter = ui->leAIMinShutter->text().toFloat();
    commonvalues::autoirisinfo[m_channel].autoiris_shutterstep = ui->leAIShutterStep->text().toFloat();
    commonvalues::autoirisinfo[m_channel].autoiris_maxgain = ui->leAIMaxGain->text().toFloat();
    commonvalues::autoirisinfo[m_channel].autoiris_mingain = ui->leAIMinGain->text().toFloat();
    commonvalues::autoirisinfo[m_channel].autoiris_gainstep = ui->leAIGainStep->text().toFloat();
    commonvalues::autoirisinfo[m_channel].autoiris_debug = ui->cbAIDebug->isChecked();
    //reflection/back
    commonvalues::autoirisinfo[m_channel].autoiris_reflection = ui->cbAIReflection->isChecked();
    commonvalues::autoirisinfo[m_channel].autoiris_backlight = ui->cbAIBackLight->isChecked();
    commonvalues::autoirisinfo[m_channel].autoiris_maxbr = ui->leAIMaxBR->text().toInt();
    commonvalues::autoirisinfo[m_channel].autoiris_minbr = ui->leAIMinBR->text().toInt();
    commonvalues::autoirisinfo[m_channel].autoiris_brstep = ui->leAIBRStep->text().toInt();
    commonvalues::autoirisinfo[m_channel].autoiris_maxplatebr = ui->leAIMaxPlateBR->text().toInt();
    commonvalues::autoirisinfo[m_channel].autoiris_minplatebr = ui->leAIMinPlateBR->text().toInt();
    commonvalues::autoirisinfo[m_channel].autoiris_brchangecount = ui->leAIBRChangeCount->text().toInt();
    commonvalues::autoirisinfo[m_channel].autoiris_reflectbackmode = ui->cmbAIReflectBackMode->currentIndex();
    //no Recog
    commonvalues::autoirisinfo[m_channel].autoiris_norecog = ui->cbAINoRecog->isChecked();
    commonvalues::autoirisinfo[m_channel].autoiris_norecogmaxbr = ui->leAINRMaxBR->text().toInt();
    commonvalues::autoirisinfo[m_channel].autoiris_norecogminbr = ui->leAINRMinBR->text().toInt();
    //config save
    AutoIrisSave();

    //apply autoiris
    ai->init();
}

void APSGDlg::on_btnLightSave_clicked()
{
    commonvalues::autolightnfo[m_channel].light_24h = ui->cbLight24H->isChecked();
    commonvalues::autolightnfo[m_channel].light_shutter = ui->leLightShutter->text().toFloat();

    float ongain = ui->leLightOnGain->text().toFloat();
    float offgain = ui->leLightOffGain->text().toFloat();
    if( offgain > ongain)
    {
        float tmpgain = offgain;
        offgain = ongain;
        ongain = tmpgain;
    }
    commonvalues::autolightnfo[m_channel].light_ongain = ongain;
    commonvalues::autolightnfo[m_channel].light_offgain = offgain;
    //config save
    LightSave();
    //apply autoiris
    ai->init();
}
