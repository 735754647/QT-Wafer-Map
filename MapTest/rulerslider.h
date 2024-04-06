#ifndef RULERSLIDER_H
#define RULERSLIDER_H


#define SMALL_X 5 //小刻度
#define MIDDLE_X 7 //中刻度
#define BIG_X 10  //大刻度


#define SMALL_Y 5 //小刻度
#define MIDDLE_Y 7 //中刻度
#define BIG_Y 10  //大刻度
#define DIFFERVALUE 1 //最小差值(最大值与最小值的差)
#define SIDEDISTANCE 0 //尺子距离左边的距离
#define SIDEDISTANCER 0 //尺子距离右边的距离
#define HANDLEWIDTH 10 //滑块的宽度
#define HANDLEHEIGHT 15 //滑块的高度
#define MININTERVSL 10 //最小刻度之间距离
#define MAXINTERVSL 20 //最大刻度之间距离
#define DISTANCEMOUSE 20 //鼠标悬停距离箭头的位置
#include <QSlider>
#include <QLabel>

class RulerSlider : public QSlider

{

public:
    Qt::Orientations orientation;

    RulerSlider(Qt::Orientation orientation, QWidget *parent = nullptr);

    ~RulerSlider();

    //刻度之间的距离

    qreal sliderInterval = 1;

    //显示刻度值

    QLabel *valueLabel;

    //滑块

    QLabel *handleLabel;

    //鼠标是否点击

    bool mouseIsClick = false;

    //设置控件的范围

    void setRulerSliderRange(int min,int max);

    //设置当前值

    void setRulerSliderValue(int value);

private:
    Qt::Orientation m_orientation;
    double m_sliderInterval;
    //绘制尺子背景

    void drawRulerBackgroud(QPainter *painter);

    //绘制刻度线与值

    void drawSliderMark(QPainter *painter);

    //根据坐标位置计算当前值

    void eventPosGetValue(QMouseEvent *ev);

    //鼠标事件

    void mouseFilterEvent(QMouseEvent *event);

protected:

    //重新绘制

    void paintEvent(QPaintEvent *);

    //过滤器

    bool eventFilter(QObject *watched,QEvent *event);

};

#endif // RULERSLIDER_H

