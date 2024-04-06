#ifndef MAPTEST_H
#define MAPTEST_H

#include <unordered_map>
#include <iostream>
#include <string>
#include <vector>
#include <QMainWindow>
#include <QtCore>
#include <QCheckBox>
#include <QStyleFactory>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QImageReader>
#include <QMessageBox>
#include <QLineEdit>
#include <QColorDialog>
#include <QRegularExpressionValidator>

#include "mylabel.h"
#include "RectangleLabel.h"
#include "rulerslider.h"
#include "drawingthread.h"

#define mapwidth  800 //大地图的宽
#define mapheight 800 //大地图的高


enum Direction {
    LeftToRightTopToBottom,
    LeftToRightBottomToTop,
    RightToLeftTopToBottom,
    RightToLeftBottomToTop,
    TopToBottomLeftToRight,
    TopToBottomRightToLeft,
    BottomToTopLeftToRight,
    BottomToTopRightToLeft
};

QT_BEGIN_NAMESPACE
namespace Ui { class MapTest; }
QT_END_NAMESPACE

class MapTest : public QMainWindow
{
	Q_OBJECT

public:
	MapTest(QWidget* parent = nullptr);
	~MapTest();

    QPixmap LoadImageFunction(const QString &imagePath,const QSize &targetsize);//加载图片
private:
	Ui::MapTest* ui;

    QTimer *m_pTimer;//定时器
    QImage Image;

    DrawingThread *m_drawingThread;

    MyLabel mylabel;// 实例化mylabel对象

    RectangleLabel rectangleLabel;// 实例化rectangleLabel对象

    RulerSlider *BigMapHorizontalslider; // 实例化大地图水平刻度尺BigMapHorizontalslider对象

    RulerSlider *BigMapVerticalslider;   // 实例化大地图垂直刻度尺BigMapVerticalslider对象

    RulerSlider *SmallMapHorizontalslider;// 实例化小地图水平刻度尺SmallMapHorizontalslider对象

    RulerSlider *SmallMapVerticalslider; // 实例化小地图垂直刻度尺SmallMapVerticalslider对象

    short int rodegrees = 0;//旋转角度

    short int Mouse_x = 0, Mouse_y = 0;//鼠标点击位置

    short int RecordCurrent_x = 0, RecordCurrent_y = 0;//记录容器当前点

    int map_columns = 0, map_rows = 0;//地图行列

    bool IsDrawSmallMap = false, IsClickSmallMap = false;//是否画小地图；点击小地图区域画小地图

    QString	filTexyename;//地图文件路径

    QMap<int, QMap<int, string>> m_mapData;//X,Y data原始地图容器

    QMap<int, QMap<int, string>> rotate_m_mapData;//旋转后地图容器

private slots:
    void handleTimeout();
	void on_read_pushButton_clicked();//读取TXT

	void on_sure_pushButton_clicked();//spinBox确定

	void on_rotate_clicked();//旋转90

    void on_tableWidget_cellClicked(int row, int column);

    void on_start_pushButton_clicked();

    void on_stop_pushButton_clicked();

private:
    void ReadMapTxtFile();//读取文件

    bool eventFilter(QObject* watched, QEvent* event) override;//事件过滤器

    void Paint();//绘制大地图

    void Paint_Secondary();//画小窗口

    void Paint_rectangle();//画小地图矩形框

    void read_image(QString filename);//读取路径图片

protected:

    virtual void mousePressEvent(QMouseEvent*) override;//重写鼠标点击
};
#endif // MAPTEST_H
