#pragma execution_character_set("utf-8")
#include "mylabel.h"

int ClickPos_x = 0;
int ClickPos_y = 0;

bool ClickFlag = false;//点击标志
bool CentreFlag = false;//绘制中心标志

MyLabel::MyLabel(QWidget *parent) :QLabel(parent)
{

}


void MyLabel::UpdateRectangle()
{
    ClickFlag = false;
    CentreFlag=true;
    update();
}

void MyLabel::paintEvent(QPaintEvent *event)
{
	QLabel::paintEvent(event);//先调用父类的paintEvent为了显示'背景'!!!  
	QPainter painter(this);
    painter.setPen(QPen(Qt::red, 3));
	
    int m_realX = ClickPos_x / 24;
    int m_realY = ClickPos_y / 24;
    if (ClickFlag)
	{
        painter.drawRect(QRect(m_realX * 24 - 2.1, m_realY * 24 - 2.1, 24, 24));
        painter.drawLine(m_realX * 24, m_realY * 24 , m_realX * 24 +20, m_realY * 24 + 20);
        painter.drawLine(m_realX * 24+20, m_realY * 24, m_realX * 24 , m_realY * 24+20 );
	}
    else if(CentreFlag)
    {
        painter.drawRect(QRect(7 * 24 - 2.1, 7 * 24 - 2.1, 24, 24));
        painter.drawLine(7 * 24, 7 * 24 , 7 * 24 +20, 7 * 24 + 20);
        painter.drawLine(7 * 24+20, 7 * 24, 7 * 24 , 7 * 24+20 );
    }
    update();
}

void MyLabel::mousePressEvent(QMouseEvent *e)
{
	QWidget::mousePressEvent(e);
    ClickPos_x = e->pos().x();
    ClickPos_y = e->pos().y();

    ClickFlag = true;
}



