#include "recognitiondlg.h"
#include "ui_recognitiondlg.h"
#include "commonvalues.h"

RecognitionDlg::RecognitionDlg(int channel, recogthread *rgthr, config *confg, QWidget *parent)  :
    QDialog(parent),
    ui(new Ui::RecognitionDlg)
{
    ui->setupUi(this);

    this->setWindowTitle(QString("Recognition Ch%1").arg(channel));
    this->setFixedSize(this->geometry().width(),this->geometry().height());

    m_channel = channel;
    recogthr = rgthr;
    cfg = confg;

    gvehicleposition= new QButtonGroup(this);
    gvehicleposition->addButton(ui->rbVPLeft,0);
    gvehicleposition->addButton(ui->rbVPCenter,1);
    gvehicleposition->addButton(ui->rbVPRight,2);


    grawsavetype = new QButtonGroup(this);
    grawsavetype->addButton(ui->rbRawsaveall,0);
    grawsavetype->addButton(ui->rbRawsavemiss,1);
    grawsavetype->addButton(ui->rbRawsaverecog,2);

    connect(ui->lblRecogimg,SIGNAL(Mouse_Pressed(int,int)),this,SLOT(Mouse_Pressed(int,int)));
    connect(ui->lblRecogimg,SIGNAL(Mouse_Released(int,int)),this,SLOT(Mouse_Released(int,int)));
    connect(ui->lblRecogimg,SIGNAL(Mouse_Right_Pressed(int,int)),this,SLOT(Mouse_Right_Pressed(int,int)));
}

RecognitionDlg::~RecognitionDlg()
{
    delete ui;
}

void RecognitionDlg::configsave()
{
    /*      Recognition             */
    QString strtitle = QString("Channel%1|RECOGNITION|PLATEAREA").arg(m_channel);
    cfg->setuint(strtitle,"sx",commonvalues::cameraSys[m_channel].recog_plateroi_sx);
    cfg->setuint(strtitle,"sy",commonvalues::cameraSys[m_channel].recog_plateroi_sy);
    cfg->setuint(strtitle,"width",commonvalues::cameraSys[m_channel].recog_plateroi_width);
    cfg->setuint(strtitle,"height",commonvalues::cameraSys[m_channel].recog_plateroi_height);
    strtitle = QString("Channel%1|RECOGNITION|BRAREA").arg(m_channel);
    cfg->setuint(strtitle,"sx",commonvalues::cameraSys[m_channel].recog_brroi_sx);
    cfg->setuint(strtitle,"sy",commonvalues::cameraSys[m_channel].recog_brroi_sy);
    cfg->setuint(strtitle,"width",commonvalues::cameraSys[m_channel].recog_brroi_width);
    cfg->setuint(strtitle,"height",commonvalues::cameraSys[m_channel].recog_brroi_height);
    strtitle = QString("Channel%1|RECOGNITION").arg(m_channel);
    cfg->setuint(strtitle,"VehiclePosition",commonvalues::cameraSys[m_channel].recog_vehicleposition);
    cfg->setuint(strtitle,"RecognitionMode",commonvalues::cameraSys[m_channel].recog_recognition_mode);
    {
        int count = sizeof( commonvalues::cameraSys[m_channel].recog_recogseq ) / sizeof( int );
        QString strval;

        for(int i=0; i< count;i++)
        {
            strval += QString::number(commonvalues::cameraSys[m_channel].recog_recogseq[i]);
        }
        cfg->set(strtitle,"RecogSeq",strval);
    }
    cfg->setuint(strtitle,"RepeatTime",commonvalues::cameraSys[m_channel].recog_recogrepeattime);
    cfg->setuint(QString("Channel%1|RECOGNITION|RAWSAVE").arg(m_channel),"Type",commonvalues::cameraSys[m_channel].recog_rawsavetype);
    cfg->setuint(QString("Channel%1|RECOGNITION|RAWSAVE").arg(m_channel),"COUNT",commonvalues::cameraSys[m_channel].recog_rawsavecount);

    cfg->setuint("LOG","RecogSaveLog",(uint)commonvalues::recogSaveLog );

    cfg->save();
}

void RecognitionDlg::configload()
{
    ui->lePlateROISX->setText( QString::number( commonvalues::cameraSys[m_channel].recog_plateroi_sx ));
    ui->lePlateROISY->setText( QString::number( commonvalues::cameraSys[m_channel].recog_plateroi_sy ));
    ui->lePlateROIWIDTH->setText( QString::number( commonvalues::cameraSys[m_channel].recog_plateroi_width ));
    ui->lePlateROIHEIGHT->setText( QString::number( commonvalues::cameraSys[m_channel].recog_plateroi_height ));
    ui->leBRROISX->setText( QString::number( commonvalues::cameraSys[m_channel].recog_brroi_sx ));
    ui->leBRROISY->setText( QString::number( commonvalues::cameraSys[m_channel].recog_brroi_sy ));
    ui->leBRROIWIDTH->setText( QString::number( commonvalues::cameraSys[m_channel].recog_brroi_width ));
    ui->leBRROIHEIGHT->setText( QString::number( commonvalues::cameraSys[m_channel].recog_brroi_height ));

    //QRadioButton *rbraw = (QRadioButton *)
    QAbstractButton *rbraw = gvehicleposition->button( commonvalues::cameraSys[m_channel].recog_vehicleposition );
    rbraw->setChecked(true);

    ui->cbRecogMode->setCurrentIndex(commonvalues::cameraSys[m_channel].recog_recognition_mode);

    int count = sizeof( commonvalues::cameraSys[m_channel].recog_recogseq ) / sizeof( int );
    QString strval;
    for(int i=0; i< count;i++)
    {
        strval += QString::number(commonvalues::cameraSys[m_channel].recog_recogseq[i]);
    }
    ui->leRecogSeq->setText(strval);

    ui->cbRecogrepeattime->setCurrentIndex( commonvalues::cameraSys[m_channel].recog_recogrepeattime - 1);

    rbraw = (QRadioButton *)grawsavetype->button( commonvalues::cameraSys[m_channel].recog_rawsavetype );
    rbraw->setChecked(true);

    ui->leRawsavetime->setText( QString::number( commonvalues::cameraSys[m_channel].recog_rawsavecount ));

    ui->chkSaveLog->setChecked(commonvalues::recogSaveLog > 0 ? true : false );

}

void RecognitionDlg::showEvent(QShowEvent *e)
{
    configload();
    display();
    QDialog::showEvent(e);
}

void RecognitionDlg::display(QImage *img)
{
    if(this->isVisible())
    {
        QPixmap pixmp;
        if( img != NULL && ui->chkDisplay->isChecked() )
        {
            pixmp = QPixmap::fromImage(*img);
            m_oldpixmap = pixmp;
        }
        else
        {
            pixmp = m_oldpixmap;
        }

        drawarea(&pixmp);
        //ui->lblRecogimg->setPixmap( QPixmap::fromImage(capimg));
        ui->lblRecogimg->setPixmap(pixmp);
        ui->lblRecogimg->show();
    }
}

void RecognitionDlg::drawarea(QPixmap *pixmp)
{
    //plate area
    QPainter painte;
    painte.begin(pixmp);
    painte.setPen(Qt::red);

    int plate_sx = ui->lePlateROISX->text().toInt();
    int plate_sy = ui->lePlateROISY->text().toInt();
    int plate_width = ui->lePlateROIWIDTH->text().toInt();
    int plate_height = ui->lePlateROIHEIGHT->text().toInt();
    painte.drawRect(plate_sx,plate_sy,plate_width,plate_height);

    painte.setPen(Qt::yellow);
    int br_sx = ui->leBRROISX->text().toInt();
    int br_sy = ui->leBRROISY->text().toInt();
    int br_width = ui->leBRROIWIDTH->text().toInt();
    int br_height = ui->leBRROIHEIGHT->text().toInt();
    painte.drawRect(br_sx,br_sy,br_width,br_height);

    if(right_x != 0 && right_y != 0)
    {
        painte.setPen(Qt::blue);
        int plate_sx = right_x;
        int plate_sy = right_y;
        int plate_width = whiteplatewidth;
        int plate_height = whiteplateheight;
        painte.drawRect(plate_sx,plate_sy,plate_width,plate_height);
        painte.setPen(Qt::green);
        plate_width = greenplatewidth;
        plate_height = greenplateheight;
        painte.drawRect(plate_sx,plate_sy,plate_width,plate_height);
    }

    painte.end();
}

void RecognitionDlg::init()
{
    max_x = ui->lblRecogimg->width();
    max_y = ui->lblRecogimg->height();
    if( max_x <= 0) max_x = 1;
    if( max_y <= 0) max_y = 1;
    multiply_width = (float)commonvalues::cameraSys[m_channel].cam_image_width / (float)max_x;
    multiply_height = (float)commonvalues::cameraSys[m_channel].cam_image_height / (float)max_y;

    //white plate pixel size(parking)
    whiteplatewidth =  300;//(int)((float)( 300) / multiply_width);
    whiteplateheight = 75;//(int)((float)( 75) / multiply_height);
    //green plate pixel size(parking)
    greenplatewidth = 200;//(int)((float)( 200) / multiply_width);
    greenplateheight = 100;//(int)((float)( 100) / multiply_height);
    right_x=0;
    right_y=0;
}

void RecognitionDlg::on_btnSave_clicked()
{
    commonvalues::cameraSys[m_channel].recog_plateroi_sx = ui->lePlateROISX->text().toInt();
    commonvalues::cameraSys[m_channel].recog_plateroi_sy = ui->lePlateROISY->text().toInt();
    commonvalues::cameraSys[m_channel].recog_plateroi_width = ui->lePlateROIWIDTH->text().toInt();
    commonvalues::cameraSys[m_channel].recog_plateroi_height = ui->lePlateROIHEIGHT->text().toInt();
    commonvalues::cameraSys[m_channel].recog_brroi_sx = ui->leBRROISX->text().toInt();
    commonvalues::cameraSys[m_channel].recog_brroi_sy = ui->leBRROISY->text().toInt();
    commonvalues::cameraSys[m_channel].recog_brroi_width = ui->leBRROIWIDTH->text().toInt();
    commonvalues::cameraSys[m_channel].recog_brroi_height = ui->leBRROIHEIGHT->text().toInt();
    commonvalues::cameraSys[m_channel].recog_vehicleposition = gvehicleposition->checkedId();
    //sdw 2017/05/07  인식모듈 모드 즉시 적용
    commonvalues::cameraSys[m_channel].recog_recognition_mode = ui->cbRecogMode->currentIndex();

    QString svalue = ui->leRecogSeq->text();
    for(int i=0 ; i < 8; i++)
    {
        int count = svalue.length();
        if( count > 8) count = 8;
        for(int i=0; i<count; i++)
        {
          commonvalues::cameraSys[m_channel].recog_recogseq[i] =  svalue[i].digitValue();
        }
    }

    commonvalues::cameraSys[m_channel].recog_recogrepeattime = ui->cbRecogrepeattime->currentIndex() + 1;
    commonvalues::cameraSys[m_channel].recog_rawsavetype = grawsavetype->checkedId();
    commonvalues::cameraSys[m_channel].recog_rawsavecount = ui->leRawsavetime->text().toInt();
    //ilpr log save
    commonvalues::recogSaveLog =  ui->chkSaveLog->isChecked() == true ? 1 : 0;

    //sdw 2016/10/10 -> 2017/05/07 configsave로 이동
    // cfg->setuint("RECOGNITION","RecognitionMode",ui->cbRecogMode->currentIndex()) ;
    configsave();

    recogthr->recogsetting();

    emit saveevent(m_channel);
}

void RecognitionDlg::Mouse_Pressed(int x, int y)
{
    if(ui->cbPlateROI->isChecked() || ui->cbBRROI->isChecked() )
    {
        qDebug() << "Mouse Pressed : x -" << x <<" y -" << y;
        mousepress_x = x;
        mousepress_y = y;
    }
}

void RecognitionDlg::Mouse_Released(int x, int y)
{
    qDebug() << "Mouse Release : x -" << x <<" y -" << y;

    if( x < 0) x=0;
    else if( x > max_x) x = max_x;

    if( y < 0) y=0;
    else if( y > max_y) y = max_y;

    if( mousepress_x >= x || mousepress_y >= y ) return;

    int sx = (int)((float)mousepress_x * multiply_width);
    int sy = (int)((float)mousepress_y * multiply_height);

    int width =  (int)((float)( x - mousepress_x) * multiply_width);
    int height = (int)((float)( y - mousepress_y) * multiply_height);

    if( ui->cbPlateROI->isChecked())
    {
        ui->lePlateROISX->setText(QString::number(sx));
        ui->lePlateROISY->setText(QString::number(sy));
        ui->lePlateROIWIDTH->setText(QString::number(width));
        ui->lePlateROIHEIGHT->setText(QString::number(height));

    }
    if( ui->cbBRROI->isChecked())
    {
        ui->leBRROISX->setText(QString::number(sx));
        ui->leBRROISY->setText(QString::number(sy));
        ui->leBRROIWIDTH->setText(QString::number(width));
        ui->leBRROIHEIGHT->setText(QString::number(height));
    }

    ui->cbPlateROI->setChecked(false);
    ui->cbBRROI->setChecked(false);

    display();
}

void RecognitionDlg::Mouse_Right_Pressed(int x, int y)
{
    qDebug() << "Right Mouse Pressed : x -" << x <<" y -" << y;

    if( x < 0) x=0;
    else if( x > max_x) x = max_x;

    if( y < 0) y=0;
    else if( y > max_y) y = max_y;

    right_x = (int)((float)x * multiply_width);
    right_y = (int)((float)y * multiply_height);

    display();
}
