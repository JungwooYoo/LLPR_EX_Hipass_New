#include "logdlg.h"
#include "ui_logdlg.h"
#include <QDebug>
#include <QDateTime>

LogDlg::LogDlg(int channel,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LogDlg)
{
    ui->setupUi(this);

    this->setWindowTitle(QString("Log Ch%1").arg(channel));
//    this->setFixedSize(this->geometry().width(),this->geometry().height());

    m_channel = channel;
    m_maxlogline = 500;

    //disable input
    ui->teLIClog->setReadOnly(true);
    ui->teCCUlog->setReadOnly(true);
    ui->teCameralog->setReadOnly(true);
    ui->teRecoglog->setReadOnly(true);
    ui->teCenterlog->setReadOnly(true);
    //all textbox display
    ui->chkCamera->setChecked(true);
    ui->chkCCU->setChecked(true);
    ui->chkCenter->setChecked(true);
    ui->chkLIC->setChecked(true);
    ui->chkRecognition->setChecked(true);

    this->setWindowTitle(QString("Log(ch%1)").arg(channel));

    connect(this,SIGNAL(logappendsignal(int ,QString )),this , SLOT(logappendslot(int , QString ) ));
}

LogDlg::LogDlg(int channel,int maxline, QWidget *parent)
{
    ui->setupUi(this);

    m_channel = channel;
    if( maxline < 50 ) m_maxlogline = 50;
    else m_maxlogline = maxline;

    //disable input
    ui->teLIClog->setReadOnly(true);
    ui->teCCUlog->setReadOnly(true);
    ui->teCameralog->setReadOnly(true);
    ui->teRecoglog->setReadOnly(true);
    ui->teCenterlog->setReadOnly(true);
    //all textbox display
    ui->chkCamera->setChecked(true);
    ui->chkCCU->setChecked(true);
    ui->chkCenter->setChecked(true);
    ui->chkLIC->setChecked(true);
    ui->chkRecognition->setChecked(true);

    this->setWindowTitle(QString("Log(ch%1)").arg(channel));

    connect(this,SIGNAL(logappendsignal(int ,QString )),this , SLOT(logappendslot(int , QString ) ));
}

LogDlg::~LogDlg()
{
    disconnect(this,SIGNAL(logappendsignal(int ,QString )),this , SLOT(logappendslot(int , QString ) ));

    delete ui;
}

void LogDlg::resizeEvent(QResizeEvent *)
{
    int width = this->width();
    int height = this->height();

    int textbox_count = 0;
    if(ui->chkLIC->isChecked())
        textbox_count++;
    if(ui->chkCCU->isChecked())
        textbox_count++;
    if(ui->chkCamera->isChecked())
        textbox_count++;
    if(ui->chkRecognition->isChecked())
        textbox_count++;
    if(ui->chkCenter->isChecked())
        textbox_count++;

    int b10count = textbox_count + 1;
    int textbox_sx = 10;
    int textbox_sy = 40;
    int textbox_height = height - 50;
    int textbox_width = ( width - ( 10 * b10count) ) / textbox_count;

    if( ui->chkLIC->isChecked())
    {
        ui->teLIClog->setVisible(true);
        ui->teLIClog->setGeometry(textbox_sx,textbox_sy,textbox_width,textbox_height);
        textbox_sx += textbox_width + 10;
    }
    else
        ui->teLIClog->setVisible(false);

    if( ui->chkCCU->isChecked())
    {
        ui->teCCUlog->setVisible(true);
        ui->teCCUlog->setGeometry(textbox_sx,textbox_sy,textbox_width,textbox_height);
        textbox_sx += textbox_width + 10;
    }
    else
        ui->teCCUlog->setVisible(false);

    if( ui->chkCamera->isChecked())
    {
        ui->teCameralog->setVisible(true);
        ui->teCameralog->setGeometry(textbox_sx,textbox_sy,textbox_width,textbox_height);
        textbox_sx += textbox_width + 10;
    }
    else
        ui->teCameralog->setVisible(false);

    if( ui->chkRecognition->isChecked())
    {
        ui->teRecoglog->setVisible(true);
        ui->teRecoglog->setGeometry(textbox_sx,textbox_sy,textbox_width,textbox_height);
        textbox_sx += textbox_width + 10;
    }
    else
        ui->teRecoglog->setVisible(false);

    if( ui->chkCenter->isChecked())
    {
        ui->teCenterlog->setVisible(true);
        ui->teCenterlog->setGeometry(textbox_sx,textbox_sy,textbox_width,textbox_height);
        textbox_sx += textbox_width + 10;
    }
    else
        ui->teCenterlog->setVisible(false);
}

void LogDlg::logappend(int index, QString data)
{    
    emit logappendsignal(index,data);
}

bool LogDlg::loglinecheck(int index)
{
    bool brtn = false;
    int count = 0;
    switch(index)
    {
        case loglic:
            {
                count = ui->teLIClog->document()->lineCount();
                if ( count > m_maxlogline )
                {
                    qDebug() << "teLIClog clear";
                    brtn = true;
                }
            }
            break;
        case logccu:
            {
                count = ui->teCCUlog->document()->lineCount();
                if ( count > m_maxlogline )
                {
                    qDebug() << "teCCUlog clear";
                    brtn = true;
                }
            }
            break;
        case logcamera:
            {
                count = ui->teCameralog->document()->lineCount();
                if ( count > m_maxlogline )
                {
                    qDebug() << "teCameralog clear";
                    brtn = true;
                }
            }
            break;
        case logrecog:
            {
                count = ui->teRecoglog->document()->lineCount();
                if ( count > m_maxlogline )
                {
                    qDebug() << "teRecoglog clear";
                    brtn = true;
                }
            }
            break;
        case logcenter:
            {
                count = ui->teCenterlog->document()->lineCount();
                if ( count > m_maxlogline )
                {
                    qDebug() << "teCenterlog clear";
                    brtn = true;
                }
            }
            break;
        default:
            break;
    }
    return brtn;
}

void LogDlg::logappendslot(int index, QString data)
{
    bool brtn = loglinecheck(index);

    QString qdt = QDateTime::currentDateTime().toString("[hh:mm:ss:zzz]") + data;

    switch(index)
    {
        case loglic:
        {
            if(brtn)
            {
                ui->teLIClog->setPlainText(qdt);
            }
            else ui->teLIClog->append(qdt);
        }
            break;
        case logccu:
        {
            if(brtn)
            {
                ui->teCCUlog->setPlainText(qdt);
            }
            else ui->teCCUlog->append(qdt);
        }
            break;
        case logcamera:
        {
            if(brtn)
            {
                ui->teCameralog->setPlainText(qdt);
            }
            else ui->teCameralog->append(qdt);
        }
            break;
        case logrecog:
        {
            if(brtn)
            {
                ui->teRecoglog->setPlainText(qdt);
            }
            else ui->teRecoglog->append(qdt);
        }
            break;
        case logcenter:
        {
            if(brtn)
            {
                ui->teCenterlog->setPlainText(qdt);
            }
            else ui->teCenterlog->append(qdt);
        }
            break;
        default:
            break;
    }
}

void LogDlg::on_chkLIC_clicked()
{
    resizeEvent(NULL);
}

void LogDlg::on_chkCCU_clicked()
{
    resizeEvent(NULL);
}

void LogDlg::on_chkCamera_clicked()
{
    resizeEvent(NULL);
}

void LogDlg::on_chkRecognition_clicked()
{
    resizeEvent(NULL);
}

void LogDlg::on_chkCenter_clicked()
{
    resizeEvent(NULL);
}
