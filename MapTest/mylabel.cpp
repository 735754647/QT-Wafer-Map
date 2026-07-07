#pragma execution_character_set("utf-8")
#include "mylabel.h"

MyLabel::MyLabel(QWidget *parent)
    : QLabel(parent)
    , ClickPos_x(0)
    , ClickPos_y(0)
    , ClickFlag(false)
    , CentreFlag(false)
{
}

void MyLabel::UpdateRectangle()
{
    ClickFlag = false;
    CentreFlag = false;
    update();
}

void MyLabel::SetSelectedCell(int x, int y)
{
    ClickPos_x = x;
    ClickPos_y = y;
    ClickFlag = false;
    CentreFlag = false;
    update();
}

void MyLabel::ClearSelection()
{
    ClickFlag = false;
    CentreFlag = false;
    update();
}

void MyLabel::paintEvent(QPaintEvent *event)
{
    QLabel::paintEvent(event);
}

void MyLabel::mousePressEvent(QMouseEvent *e)
{
    QLabel::mousePressEvent(e);
}
