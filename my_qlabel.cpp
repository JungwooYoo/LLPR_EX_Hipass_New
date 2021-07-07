#include "my_qlabel.h"

my_qlabel::my_qlabel(QWidget *parent) :
    QLabel(parent)
{

}

void my_qlabel::mousePressEvent(QMouseEvent *ev)
{
    int x,y;
    x = ev->x();
    y = ev->y();

    if(ev->button() == Qt::RightButton)
    {
        emit Mouse_Right_Pressed(x,y);
    }
    else
    {
        emit Mouse_Pressed(x,y);
    }
}

void my_qlabel::mouseReleaseEvent(QMouseEvent *ev)
{
    int x,y;
    x = ev->x();
    y = ev->y();

    emit Mouse_Released(x,y);

}
