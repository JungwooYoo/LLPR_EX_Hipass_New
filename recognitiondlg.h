#ifndef RECOGNITIONDLG_H
#define RECOGNITIONDLG_H

#include <QDialog>
#include <QButtonGroup>
#include <QPainter>
#include <QColor>

#include "recogthread.h"
#include "config.h"

namespace Ui {
class RecognitionDlg;
}

class RecognitionDlg : public QDialog
{
    Q_OBJECT

public:
    explicit RecognitionDlg(int channel, recogthread *rgthr, config *confg,QWidget *parent = 0);
    ~RecognitionDlg();
    void configsave();
    void configload();
    void showEvent(QShowEvent *e);
    void display(QImage *img = NULL);
    void drawarea(QPixmap *pixmp);
    void init();
signals:
    void saveevent(int channel);
private slots:
    void on_btnSave_clicked();
    void Mouse_Pressed(int x, int y);
    void Mouse_Released(int x, int y);
    void Mouse_Right_Pressed(int x,int y);

private:
    Ui::RecognitionDlg *ui;
    recogthread *recogthr;
    config *cfg;
    QButtonGroup *gvehicleposition;
    QButtonGroup *grawsavetype;


    int m_channel;
    int max_x;
    int max_y;
    float multiply_width;
    float multiply_height;
    int mousepress_x;
    int mousepress_y;
    //QImage *oldimg;
    QPixmap m_oldpixmap;


    int whiteplatewidth;
    int whiteplateheight;
    int greenplatewidth;
    int greenplateheight;
    int right_x;
    int right_y;
};

#endif // RECOGNITIONDLG_H
