#include "databasedlg.h"
#include "ui_databasedlg.h"
#include "commonvalues.h"
#include <QMessageBox>

DatabaseDlg::DatabaseDlg(dbmng *db, config *cfg, QWidget *parent)  :
    QDialog(parent),
    ui(new Ui::DatabaseDlg)
{
    ui->setupUi(this);

    this->setWindowTitle(QString("DataBase"));
    this->setFixedSize(this->geometry().width(),this->geometry().height());
    ui->lePassword->setEchoMode(QLineEdit::Password);

    pdatabase = db;
    pcfg = cfg;
}

DatabaseDlg::~DatabaseDlg()
{
    delete ui;
}

void DatabaseDlg::showEvent(QShowEvent *e)
{
    ui->leIP->setText(commonvalues::databaseinfo.db_ip);
    ui->lePort->setText(QString::number(commonvalues::databaseinfo.db_port));
    ui->leUserID->setText(commonvalues::databaseinfo.db_user);
    ui->lePassword->setText(commonvalues::databaseinfo.db_password);
    ui->leConnectionTimeout->setText(QString::number(commonvalues::databaseinfo.db_connectionTimeout));
    ui->leCommandTimeout->setText(QString::number(commonvalues::databaseinfo.db_commandTimeout));
    ui->leStorageDuration->setText(QString::number(commonvalues::databaseinfo.db_storageDuration));
}

void DatabaseDlg::on_btnSave_clicked()
{
    QString strip  = ui->leIP->text().trimmed();
    int port = ui->lePort->text().trimmed().toInt();
    QString struser = ui->leUserID->text().trimmed();
    QString strpasswd = ui->lePassword->text().trimmed();
    int connectiontimeout = ui->leConnectionTimeout->text().trimmed().toInt();
    int commandtimeout = ui->leCommandTimeout->text().trimmed().toInt();
    int stroageduration = ui->leStorageDuration->text().trimmed().toInt();

    if( port == 0 || commandtimeout < 1 || connectiontimeout < 1 || strip.count(".") != 3 )
    {
        qDebug() << QString("DataBase Setting error");
        return;
    }
    commonvalues::databaseinfo.db_ip = strip;
    commonvalues::databaseinfo.db_port = port;
    commonvalues::databaseinfo.db_user = struser;
    commonvalues::databaseinfo.db_password = strpasswd;
    commonvalues::databaseinfo.db_connectionTimeout = connectiontimeout;
    commonvalues::databaseinfo.db_commandTimeout = commandtimeout;
    commonvalues::databaseinfo.db_storageDuration = stroageduration;

    /*        Database  value            */
    pcfg->set("DATABASE","IP",commonvalues::databaseinfo.db_ip);
    //commonvalues::db_ip = "127.0.0.1";
    pcfg->setuint("DATABASE","Port", commonvalues::databaseinfo.db_port);
    //commonvalues::db_port = 3306;
    pcfg->set("DATABASE","Name",commonvalues::databaseinfo.db_name);
    pcfg->set("DATABASE","User",commonvalues::databaseinfo.db_user);
    //commonvalues::db_user = "root";
    pcfg->set("DATABASE","Password",commonvalues::databaseinfo.db_password);
    //commonvalues::db_password = "ubuntu";
    pcfg->setuint("DATABASE","ConnectionTimeout",commonvalues::databaseinfo.db_connectionTimeout);
    pcfg->setuint("DATABASE","CommandTimeout",commonvalues::databaseinfo.db_commandTimeout);
    pcfg->setuint("DATABASE","StorageDuration",commonvalues::databaseinfo.db_storageDuration);

    pcfg->save();

    pdatabase->disconnect();
    pdatabase->init();  //init and connection
}

void DatabaseDlg::on_btnCancel_clicked()
{
    this->hide();
}

void DatabaseDlg::on_btnOBUDBDelete_clicked()
{
    QString strinfo;
    if(pdatabase->dropdb(pdatabase->OBUdbname))
    {
        strinfo = QString("Drop OBU Database OK");
    }
    else
        strinfo = QString("Drop OBU Database Fail");

    QMessageBox::information(this,"info",strinfo);
}

void DatabaseDlg::on_btnDBDelete_clicked()
{
    QString strinfo;
    if(pdatabase->dropdb(pdatabase->dbname))
    {
        strinfo = QString("Drop Database OK");
    }
    else
        strinfo = QString("Drop Database Fail");
}
