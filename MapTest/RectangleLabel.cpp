#include "RectangleLabel.h"

double rectWidth;
double rectHeight;
bool isNewDrawing;
bool isDrawing;
QRectF m_rec;

RectangleLabel::RectangleLabel(QWidget *parent) :QLabel(parent)
  {
      isDrawing = false;
      rectWidth = 45; // 默认宽度
      rectHeight = 45; // 默认高度
  }


void RectangleLabel::UpdateRectangle(const QRectF rec)
{
    m_rec=rec;
    isDrawing=false;
    isNewDrawing=true;
    update();
}

void RectangleLabel::setRectangle(double width,double height)
{

    rectWidth = width;
    rectHeight = height;
    update();
}

void RectangleLabel::paintEvent(QPaintEvent *event)
{
    QLabel::paintEvent(event);

    if (isDrawing)
    {
        QPainter painter(this);
        painter.setPen(QPen(Qt::red, 3));
        painter.drawRect(rectangle);
    }
    else if(isNewDrawing)
    {
        QPainter painter(this);
        painter.setPen(QPen(Qt::red, 3));
        painter.drawRect(m_rec);
    }

}

void RectangleLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        QPointF center = event->pos();
        QPointF topLeft(center.x() - rectWidth / 2, center.y() - rectHeight / 2);
        QPointF bottomRight(center.x() + rectWidth / 2, center.y() + rectHeight / 2);
        rectangle = QRectF(topLeft, bottomRight);
        isDrawing = true;
        update();
    }

    QLabel::mousePressEvent(event);
}
