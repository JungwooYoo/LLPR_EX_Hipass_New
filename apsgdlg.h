#ifndef APSGDLG_H
#define APSGDLG_H

#include <QDialog>

#include "config.h"
#include "autoiris.h"

namespace Ui {
class APSGDlg;
}

class APSGDlg : public QDialog
{
    Q_OBJECT

public:
    explicit APSGDlg(int channel, autoiris *ati,config *pcfg,QWidget *parent = 0);
    ~APSGDlg();
    void showEvent(QShowEvent *e);
    void AutoIrisGetsetting();
    void LightGetsetting();
    void AutoIrisSave();
    void LightSave();

private slots:
    void on_btnAISave_clicked();
    void on_btnLightSave_clicked();

private:
    Ui::APSGDlg *ui;
    autoiris *ai;
    config *cfg;
    int m_channel;
};

#endif // APSGDLG_H
