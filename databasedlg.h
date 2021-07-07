#ifndef DATABASEDLG_H
#define DATABASEDLG_H

#include <QDialog>

#include "config.h"
#include "dbmng.h"


namespace Ui {
class DatabaseDlg;
}

class DatabaseDlg : public QDialog
{
    Q_OBJECT

public:
    explicit DatabaseDlg(dbmng *db,config *cfg ,QWidget *parent = 0);
    ~DatabaseDlg();

    void showEvent(QShowEvent *e);
private slots:
    void on_btnSave_clicked();

    void on_btnCancel_clicked();

    void on_btnOBUDBDelete_clicked();

    void on_btnDBDelete_clicked();

private:
    Ui::DatabaseDlg *ui;
    dbmng *pdatabase;
    config *pcfg;
};

#endif // DATABASEDLG_H
