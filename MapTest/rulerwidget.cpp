
#include "rulerwidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QTimer>

RulerWidget::RulerWidget(Qt::Orientations orientation,QWidget *parent)
    : QWidget(parent),
      orientation(orientation)
{
    orientation == Qt::Horizontal ? setFixedHeight(60) : setFixedWidth(100);
    slidingLineColor.setAlphaF(0.7);
}

RulerWidget::~RulerWidget()
{
}

void RulerWidget::paintEvent(QPaintEvent *event)
{
    auto rect = event->rect();
        auto height = rect.height();
        auto width = rect.width();

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect, backgroundColor);

        painter.save();
        painter.setFont(font);

        if (orientation == Qt::Horizontal) {
            painter.translate(0 - offset, 0);

            auto shortLine_y = height * 0.75;
            auto longLine_y = height * 0.5;
            auto middleLine_y = height * 0.625;
            auto halfHeight = height / 2;
            int temp = 0;

            for (int i = 0; i < width + offset; i += 10) {
                if (temp % 10 == 0) {
                    painter.drawLine(QPointF(i, middleLine_y), QPointF(i, height));
                } else if (temp % 5 == 0) {
                    painter.drawLine(QPointF(i, longLine_y), QPointF(i, height));
                    painter.drawText(QRectF(i - 50, 0, 100, halfHeight), Qt::AlignCenter | Qt::TextWordWrap, QString::number(i));
                } else {
                    painter.drawLine(QPointF(i, shortLine_y), QPointF(i, height));
                }
                ++temp;
            }
        } else {
            painter.translate(0, 0 - offset);

            auto shortLineWidth = width * 0.25;
            auto longLineWidth = width * 0.5;
            auto middleLineWidth = width * 0.375;
            auto halfWidth = width / 2;
            int temp = 0;

            for (int i = 0; i < height + offset; i += 10) {
                if (temp % 10 == 0) {
                    painter.drawLine(QPointF(middleLineWidth, i), QPointF(0, i));
                } else if (temp % 5 == 0) {
                    painter.drawLine(QPointF(longLineWidth, i), QPointF(0, i));
                    painter.drawText(QRectF(halfWidth, i - 50, halfWidth, 100), Qt::AlignCenter | Qt::TextWordWrap, QString::number(i));
                } else {
                    painter.drawLine(QPointF(shortLineWidth, i), QPointF(0, i));
                }
                ++temp;
            }
        }

        painter.restore();

        painter.setPen(Qt::transparent);
        painter.setBrush(slidingLineColor);

        if (orientation == Qt::Horizontal) {
            painter.drawRect(slidingLinePos, 0, 3, height);
        } else {
            painter.drawRect(0, slidingLinePos, width, 3);
        }

        QWidget::paintEvent(event);
}
