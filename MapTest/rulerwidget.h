#ifndef RULERWIDGET_H
#define RULERWIDGET_H

#include <QWidget>

class RulerWidget : public QWidget
{
    Q_OBJECT

public:
    RulerWidget(Qt::Orientations orientation, QWidget *parent = nullptr);
    ~RulerWidget()override;
    void setOffset(int value)
    {
        offset = value;
        update();
    }
    void setSlidingLinePos(int pos)
    {
        slidingLinePos = pos + offset;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event)override;

private:
    QColor backgroundColor{Qt::white};//背景色
    QColor textAndLineColor{"#606060"};//文本和刻度颜色
    QColor slidingLineColor{"#D56161"};//游标颜色
    Qt::Orientations orientation;
    int slidingLinePos{0};
    int offset{0};
    QFont font{"微软雅黑",16};
};

#endif // RULERWIDGET_H
