#include "rulerslider.h"
#include "QPainter"
#include "math.h"

#include <QDebug>
#include <QEvent>
#include <QMouseEvent>

RulerSlider::RulerSlider(Qt::Orientation orientation, QWidget *parent)
    : QSlider(parent), orientation(orientation)

{
    //注册过滤器

    installEventFilter(this);

    //鼠标事件追踪

    setMouseTracking(true);

    //显示当前值label

    valueLabel = new QLabel(this);

    valueLabel->setStyleSheet("background-color: rgb(0, 0, 0);font-size:14px;color:white;border:1px solid black");

    valueLabel->setAlignment(Qt::AlignCenter);

    valueLabel->hide();

    //滑块

    handleLabel = new QLabel(this);

    handleLabel->setFixedSize(20,15);

    handleLabel->raise();

    QImage image,result;

    image.load(":/images/handle.png");//temppath为图片的路径

    result = image.scaled(handleLabel->width(), handleLabel->height(),Qt::IgnoreAspectRatio, Qt::SmoothTransformation);//放缩图片，以固定大小显示

    handleLabel->setPixmap(QPixmap::fromImage(result));//在Label控件上显示图片

}

RulerSlider::~RulerSlider()

{

}

/**

* 绘制

* @brief RulerSlider::paintEvent

*/

void RulerSlider::paintEvent(QPaintEvent *)

{

    QPainter painter;

    painter.begin(this);

    //抗锯齿

    painter.setRenderHint(QPainter::Antialiasing);

    drawRulerBackgroud(&painter);

    drawSliderMark(&painter);

    painter.end();

}

/**

* 绘制尺子背景

* @brief RulerSlider::drawRulerBackgroud

* @param painter

*/

void RulerSlider::drawRulerBackgroud(QPainter *painter)

{

    QPointF topLeftPot3(0,0);

    QPointF bottomRightPot3(width(),height());

    painter->setPen(Qt::white);

    painter->setBrush(QColor(61,84,110));

    painter->drawRect(QRectF(topLeftPot3,bottomRightPot3));

}

/**

* 绘制刻度与值

* @brief RulerSlider::drawSliderMark

* @param painter

*/

void RulerSlider::drawSliderMark(QPainter *painter)

{

    painter->setPen(Qt::gray);

    painter->setBrush(QColor(128,128,128));

    //计算刻度之间的间隔最小为10

    int dis = maximum()-minimum();


    int SpaceNum;
    if(dis>800)
    {
        SpaceNum=40;
    }
    else if(dis>600)
    {
        SpaceNum=30;
    }
    else if(dis>300)
    {
        SpaceNum=20;
    }
    else
    {
        SpaceNum=10;
    }


    //绘制横坐标开始起点
    if(orientation == Qt::Horizontal)
    {
        qreal startX = SIDEDISTANCE;

       // for (uint16_t i=minimum();i<=(minimum()+dis/singleStep());i++) {
        for (int i=minimum();i <= maximum(); i += singleStep()) {

            //偶数          

            if(i%SpaceNum==0)
            {

                painter->drawRect(QRectF(startX,0,0.1,BIG_Y));

                //绘制刻度值

                float value = round(minimum()+((i-minimum())*singleStep()));

                QString strValue = tr("%1").arg(value);

                int fontWidth = fontMetrics().width(strValue)/2;

                painter->drawText(QPointF(startX-fontWidth,BIG_Y+15),strValue);

            }
            else
            {

                if(i%5 == 0){

                    painter->drawRect(QRectF(startX,0,0.1,MIDDLE_Y));

                }else{

                    painter->drawRect(QRectF(startX,0,0.1,SMALL_Y));


                }

            }

            if(dis<20)
            {
                if(minimum()<85)
                {
                    float value = round(minimum()+((i-minimum())*singleStep()));

                    QString strValue = tr("%1").arg(value);

                    int fontWidth = fontMetrics().width(strValue)/2;

                    painter->drawText(QPointF(startX-fontWidth,BIG_Y+15),strValue);
                }
                else if(maximum()>99&&i%2 == 0)
                {
                    float value = round(minimum()+((i-minimum())*singleStep()));

                    QString strValue = tr("%1").arg(value);

                    int fontWidth = fontMetrics().width(strValue)/2;

                    painter->drawText(QPointF(startX-fontWidth,BIG_Y+15),strValue);
                }
            }

            startX+=sliderInterval;

        }

    }
    else
    {
        qreal startY = SIDEDISTANCE;

        for (int i = minimum(); i <= maximum(); i += singleStep())
        {
            if (i % SpaceNum == 0)
            {
                painter->drawRect(QRectF(0, startY, BIG_X, 0.1));

                float value = round(minimum() + ((i - minimum()) * singleStep()));
                QString strValue = tr("%1").arg(value);
                int fontHeight = fontMetrics().height() / 2;
                painter->drawText(QPointF(BIG_X , startY + fontHeight), strValue);
            }
            else
            {
                if (i % 5 == 0) {
                    painter->drawRect(QRectF(0, startY, MIDDLE_X, 0.1));
                } else {
                    painter->drawRect(QRectF(0, startY, SMALL_X, 0.1));
                }
            }

            if(dis<20)
            {
                if(minimum()<85)
                {
                    float value = round(minimum() + ((i - minimum()) * singleStep()));
                    QString strValue = tr("%1").arg(value);
                    int fontHeight = fontMetrics().height() / 2;
                    painter->drawText(QPointF(BIG_X , startY + fontHeight), strValue);
                }
                else if(maximum()>99&&i%2 == 0)
                {
                    float value = round(minimum() + ((i - minimum()) * singleStep()));
                    QString strValue = tr("%1").arg(value);
                    int fontHeight = fontMetrics().height() / 2;
                    painter->drawText(QPointF(BIG_X , startY + fontHeight), strValue);
                }
            }

            startY += sliderInterval;
        }

    }
}
/**

* 根据鼠标位置获取当前值

* @brief RulerSlider::eventPosGetValue

* @param ev

*/

void RulerSlider::eventPosGetValue(QMouseEvent *ev)

{
    if(orientation == Qt::Horizontal)
    {
    //光标所在矩形范围

    double posWidth = (double)width()-SIDEDISTANCER-SIDEDISTANCE;

    //鼠标光标位置

    double xpos = ev->pos().x();

    if(xpos >=SIDEDISTANCE&&xpos<=width()-SIDEDISTANCER){

        //根据光标计算

        double posValue = (xpos-SIDEDISTANCE)/posWidth;

        int value =round(posValue * (maximum()-minimum()) + minimum());

        handleLabel->move(xpos-handleLabel->width()/2,0);

        qDebug()<<"结果值"<<value;

        setValue(value);

    }
    }
    else
    {
        double posHeight = height() - SIDEDISTANCER - SIDEDISTANCE;
           double ypos = ev->pos().y();

           if (ypos >= SIDEDISTANCE && ypos <= height() - SIDEDISTANCER) {
               double posValue = (ypos - SIDEDISTANCE) / posHeight;
               int value = qRound(posValue * (maximum() - minimum()) + minimum());
               handleLabel->move(0, ypos - handleLabel->height() / 2);
               setValue(value);
           }
    }
}

/**

* 设置slider范围

* @brief RulerSlider::setRulerSliderRange

* @param min

* @param max

*/

void RulerSlider::setRulerSliderRange(int min, int max)

{

    //当设置的最小值为负数或0时，设置最小值为0

    int minNum = min;

    int maxNum = max;

//    if(min<=0){

//        minNum = 0;

//    }

    //当最大值与最小值之间差值

    if(max-min <= DIFFERVALUE){

        maxNum = min+DIFFERVALUE;

    }

    qDebug()<<"滑动条"<<value()<<minNum<<maxNum;

    setMaximum(maxNum);

    setMinimum(minNum);


    if(orientation == Qt::Horizontal)
    {
        if(value()>minNum){

            int dis = maxNum-minNum;

            sliderInterval = (float)(width()-SIDEDISTANCE-SIDEDISTANCER)/(dis/singleStep());

            double pos = ((value()-minNum)/singleStep())*sliderInterval+SIDEDISTANCE;

            handleLabel->move(pos-handleLabel->width()/2,0);

        }
        else
        {

            handleLabel->move(SIDEDISTANCE-handleLabel->width()/2,0);

        }
    }
    else
    {
        if(value()>minNum){

            int dis = maxNum-minNum;

            sliderInterval = (float)(height()-SIDEDISTANCE-SIDEDISTANCER)/(dis/singleStep());

            double pos = ((value()-minNum)/singleStep())*sliderInterval+SIDEDISTANCE;

            handleLabel->move(pos-handleLabel->height()/2,0);

        }
        else
        {

            handleLabel->move(SIDEDISTANCE-handleLabel->height()/2,0);

        }
    }
    update();

}

/**

* 设置当前值

* @brief RulerSlider::setRulerSliderValue

* @param value

*/

void RulerSlider::setRulerSliderValue(int value)

{

    if(orientation == Qt::Horizontal)
    {
        int dis = maximum()-minimum();

        sliderInterval = (float)(width()-SIDEDISTANCE-SIDEDISTANCER)/(dis/singleStep());

        double pos = ((value-minimum())/singleStep())*sliderInterval+SIDEDISTANCE;

        handleLabel->move(pos-handleLabel->width()/2,0);

        setValue(value);
    }
    else
    {
        int dis = maximum() - minimum();

        sliderInterval = (float)(height() - SIDEDISTANCE - SIDEDISTANCER) / (dis / singleStep());

        double pos = ((value - minimum()) / singleStep()) * sliderInterval + SIDEDISTANCE;

        handleLabel->move(0, pos - handleLabel->height() / 2);

        setValue(value);

    }
}

/**

* 鼠标移动事件

* 当mouseIsClick为true时拖动false悬停

* @brief RulerSlider::mouseFilterEvent

* @param event

*/

void RulerSlider::mouseFilterEvent(QMouseEvent *event)

{
    if(orientation == Qt::Horizontal)
{
    //鼠标光标位置

    double xpos = event->pos().x();

    if(xpos >=SIDEDISTANCE&&xpos<=width()-SIDEDISTANCER){

        if(mouseIsClick){

            handleLabel->move(xpos-handleLabel->width()/2,0);

            if(valueLabel->isVisible()){

                valueLabel->hide();

            }

        }else {

            //光标所在矩形范围

            double posWidth = (double)width()-SIDEDISTANCER-SIDEDISTANCE;

            //根据光标计算

            double posValue = (xpos-SIDEDISTANCE)/posWidth;

            int value =round(posValue * (maximum()-minimum()) + minimum());

            if(valueLabel->isHidden()){

                valueLabel->show();

            }

            if(value >= minimum() && value <= maximum()){

                valueLabel->setText(QString::number(value));

                valueLabel->adjustSize();

                valueLabel->move(xpos+DISTANCEMOUSE,height()-valueLabel->height());

                valueLabel->raise();

            }

        }

    }
}
    else
    {
        double ypos = event->pos().y();

        if (ypos >= SIDEDISTANCE && ypos <= height() - SIDEDISTANCER) {
                if (mouseIsClick) {
                    handleLabel->move(0, ypos - handleLabel->height() / 2);
                    if (valueLabel->isVisible()) {
                        valueLabel->hide();
                    }
                } else {
                    double posHeight = height() - SIDEDISTANCER - SIDEDISTANCE;
                    double posValue = (ypos - SIDEDISTANCE) / posHeight;
                    int value = qRound(posValue * (maximum() - minimum()) + minimum());

                    if (valueLabel->isHidden()) {
                        valueLabel->show();
                    }

                    if (value >= minimum() && value <= maximum()) {
                        valueLabel->setText(QString::number(value));
                        valueLabel->adjustSize();
                        valueLabel->move(width() - valueLabel->width(), ypos + DISTANCEMOUSE);
                        valueLabel->raise();
                    }
                }
            }
    }
}
/*

* 事件过滤

* @brief RulerSlider::eventFilter

* @param watched

* @param event

* @return

*/

bool RulerSlider::eventFilter(QObject *watched, QEvent *event)

{

    //鼠标事件

    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

    //鼠标离开

    if(QEvent::HoverLeave == event->type()&&!valueLabel->isHidden()){

        valueLabel->hide();

    }

    //鼠标悬停与拖动

    if(QEvent::HoverMove == event->type()){

        mouseFilterEvent(mouseEvent);

    }

    //鼠标点击事件

    if(QEvent::MouseButtonPress == event->type()){

        if(mouseEvent->button() & Qt::LeftButton){

            mouseIsClick = true;

        }

    }

    //鼠标释放事件

    if(QEvent::MouseButtonRelease == event->type()){

        //注意应先调用父类的鼠标点击处理事件，这样可以不影响拖动的情况

        QSlider::mouseReleaseEvent(mouseEvent);

        eventPosGetValue(mouseEvent);

        mouseIsClick = false;

    }

    return QSlider::eventFilter(watched,event);

}
