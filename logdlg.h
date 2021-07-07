#ifndef LOGDLG_H
#define LOGDLG_H

#include <QDialog>

namespace Ui {
class LogDlg;
}

class LogDlg : public QDialog
{
    Q_OBJECT

public:
    explicit LogDlg(int channel,QWidget *parent = 0);
    explicit LogDlg(int channel, int maxline, QWidget *parent = 0);
    ~LogDlg();

    void resizeEvent(QResizeEvent*);
    void logappend(int index,QString data);
    bool loglinecheck(int index);

signals :
    void logappendsignal(int index, QString data);
public slots:
    void logappendslot(int index, QString data);
private slots:
    void on_chkLIC_clicked();
    void on_chkCCU_clicked();
    void on_chkCamera_clicked();
    void on_chkRecognition_clicked();
    void on_chkCenter_clicked();

private:
    Ui::LogDlg *ui;
    int m_maxlogline;
    int m_channel;


public:
    enum logindex
    {
        loglic = 0,
        logccu = 1,
        logcamera = 2,
        logrecog = 3,
        logcenter = 4
    };

};

#endif // LOGDLG_H
