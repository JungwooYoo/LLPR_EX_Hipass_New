#ifndef MY_QLABEL_H
#define MY_QLABEL_H

#include <QLabel>
#include <QMouseEvent>

class my_qlabel : public QLabel
{
    Q_OBJECT
public:
    explicit my_qlabel(QWidget *parent = 0);
    void mousePressEvent(QMouseEvent *ev);
    void mouseReleaseEvent(QMouseEvent *ev);

signals:
    void Mouse_Pressed(int x, int y);
    void Mouse_Released(int x, int y);
    void Mouse_Right_Pressed(int x,int y);

public slots:


};
#endif // MY_QLABEL_H
