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

enum MapEditSelectionMode {
    MapEditPoint,
    MapEditRect,
    MapEditCircle
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

    double m_scaleFactor=1.0; // 当前的缩放因子
    const double m_minScaleFactor = 1.0;
    const double m_maxScaleFactor = 16.0;
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

    int Mouse_x = 0, Mouse_y = 0;//鼠标点击位置
    QPoint m_dragStartPosition;
    QPoint m_lastDragPosition;
    QPointF m_viewCenterCell;
    bool m_isDragging = false;
    bool m_dragMoved = false;
    int RecordCurrent_x = 0, RecordCurrent_y = 0;//记录容器当前点
    int BigMapX=0 ,BigMapY=0;//大图点击位置
    int map_columns = 0, map_rows = 0;//地图行列

    bool IsDrawSmallMap = false, IsClickSmallMap = false;//是否画小地图；点击小地图区域画小地图

    QString	filTexyename;//地图文件路径

    QMap<int, QMap<int, string>> m_mapData;//X,Y data原始地图容器

    QMap<int, QMap<int, string>> rotate_m_mapData;//旋转后地图容器

    Direction m_selectedDirection = LeftToRightTopToBottom;//当前行进方向
    int m_currentMinorDx = 0;//当前次方向X，蛇形换行后会更新
    int m_currentMinorDy = 0;//当前次方向Y，蛇形换列后会更新
    bool m_hasStartPoint = false;//是否已经设置起点
    QPoint m_startPointSource;//起点，使用原始地图坐标保存
    QVector<QPoint> m_referencePointsSource;//参考点，使用原始地图坐标保存
    MapEditSelectionMode m_editSelectionMode = MapEditPoint;//地图编辑选择方式
    bool m_hasEditRectStart = false;//框选左上点是否已设置
    bool m_hasEditRectEnd = false;//框选右下点是否已设置
    QPoint m_editRectStartDisplay;//框选左上点，使用当前显示坐标保存
    QPoint m_editRectEndDisplay;//框选右下点，使用当前显示坐标保存
    bool m_hasLoadRange = false;//是否使用手动/确认后的加载范围
    int m_loadStartRow = 1;//加载起始行，1基
    int m_loadEndRow = 1;//加载结束行，1基
    int m_loadStartColumn = 1;//加载起始列，1基
    int m_loadEndColumn = 1;//加载结束列，1基
    int m_loadBinCharCount = 1;//自定义加载时每个bin占用字符数
    int m_activeLoadMode = 0;//当前加载方式
    int m_csvDataStartRow = 1;//csv加载数据起始行
    int m_csvBinColumn = 1;//csv加载bin列
    int m_csvXColumn = 1;//csv加载X坐标列
    int m_csvYColumn = 1;//csv加载Y坐标列
    int m_excelPageIndex = 0;//Excel加载页索引
    QString m_lastScanSearchPath;//手动扫码上次选择的搜索目录
    QVector<QPoint> m_pendingPathDisplayCells;//启动后待走的显示坐标路径
    QSet<qint64> m_assistPathSourceCells;//借助路径，保存原始地图坐标

private slots:
    void handleTimeout();
	void on_read_pushButton_clicked();//读取TXT

	void on_sure_pushButton_clicked();//spinBox确定

	void on_rotate_clicked();//旋转90

    void on_tableWidget_cellClicked(int row, int column);

    void on_start_pushButton_clicked();

    void on_stop_pushButton_clicked();

    void on_direction_pushButton_clicked();

private:

    void ReadMapTxtFile();//读取文件

    bool configureMapLoadRange(const QString& filePath);//加载前设置地图行列范围

    bool configureCsvLoadOptions(const QString& filePath);//csv加载参数设置

    bool configureExcelLoadRange(const QString& filePath);//Excel加载参数设置

    bool loadMapFileFromPath(const QString& filePath, int loadMode = 0);//按文件路径加载地图

    bool loadCsvMapTxtFile();//按csv列配置读取文件

    bool loadExcelMapFile();//按Excel单元格范围读取文件

    void finishLoadedMapData(const std::unordered_map<std::string, int>& stringCounts);//地图读取后刷新表格和视图

    QString findMapFileByScanText(const QString& scanText, const QStringList& searchRoots) const;//按扫码内容查找地图文件

    bool eventFilter(QObject* watched, QEvent* event) override;//事件过滤器

    void Paint();//绘制大地图

    void Paint_Secondary();//画小窗口

    void read_image(QString filename);//读取路径图片

    void resetMapScale();//恢复原图

    void DrawSmallMap(QLabel* targetLabel, int centerX, int centerY);//绘制小图

    int displayColumns() const;//当前显示方向下的列数

    int displayRows() const;//当前显示方向下的行数

    const QMap<int, QMap<int, string>>& currentMapData() const;//按当前旋转角度返回绘图用地图

    std::unordered_map<std::string, QColor> buildColorMap() const;//从表格读取bin到颜色的映射

    std::unordered_map<std::string, int> checkedTargetRows() const;//获取表格中勾选为加工目标的bin

    bool processCurrentCellIfChecked();//当前格是加工目标时标记为已加工

    QRectF currentViewRect() const;//大图当前可见的地图坐标范围

    QPoint cellFromMainPoint(const QPoint& pos) const;//大图像素坐标转显示地图坐标

    QPoint cellFromSmallPoint(const QPoint& pos) const;//小图像素坐标转显示地图坐标

    QPoint displayToSourceCell(int x, int y) const;//显示坐标转原始地图坐标

    QPoint sourceToDisplayCell(int x, int y) const;//原始地图坐标转当前显示坐标

    int clampToRange(int value, int minValue, int maxValue) const;//整数范围限制

    double clampToRange(double value, double minValue, double maxValue) const;//浮点范围限制

    qint64 sourceCellKey(int x, int y) const;//原始地图坐标生成唯一key

    bool isAssistPathDisplayCell(int displayX, int displayY) const;//判断显示坐标是否为借助路径

    void markAssistPathCell(int displayX, int displayY);//将显示坐标标记为灰色借助路径

    void setSelectedCell(int x, int y, bool centerView, bool refresh = true);//更新当前选中格

    void centerViewOnCell(int x, int y);//把大图视图中心移动到指定格

    void zoomMainMap(const QPoint& anchor, double factor);//按鼠标锚点缩放大图

    void panMainMap(const QPoint& pixelDelta);//拖拽平移大图

    void renderMainMap();//重绘大图

    void renderSmallMap();//重绘小图

    void refreshMapViews();//刷新大小图、坐标信息和刻度

    void updateSelectionUi();//刷新大图下方文件/行列/角度信息

    void updatePointAlignmentUi();//刷新起点和参考点坐标显示

    void updateRulers();//根据当前视图刷新刻度尺

    void rebuildRotatedMap();//根据旋转角度重建显示地图

    bool advanceSearchStep();//启动后推进一步加工/借助路径

    void updateDirectionButton();//刷新方向按钮文字和图标

    void setupMapFunctionPanel();//初始化右侧Map功能区信号

    void setMapFunctionPanelRunning(bool running);//启动运行时锁定相关控件

    void setStartPointFromSelection();//把当前选中格设置为起点

    void addReferencePointFromSelection();//把当前选中格加入参考点

    void clearReferencePoints();//清空参考点

    void drawMainMapMarkers(QPainter& painter, const QRectF& view, double cellWidth, double cellHeight);//绘制大图起点/参考点

    void drawSmallMapMarkers(QPainter& painter, int startX, int startY, double cellWidth, double cellHeight, int cellCount);//绘制小图起点/参考点

    void updateEditSelectionUi();//刷新Map编辑坐标显示

    void setEditSelectionMode(MapEditSelectionMode mode);//切换点选/框选/圆选

    void setEditRectStartFromSelection();//当前点作为框选左上点

    void setEditRectEndFromSelection();//当前点作为框选右下点

    QVector<QPoint> editSelectionSourceCells() const;//获取当前编辑选区对应的原始地图坐标

    void applyEditToSelection(const QString& newValue);//修改或裁剪当前编辑选区

    void drawMainMapEditSelection(QPainter& painter, const QRectF& view, double cellWidth, double cellHeight);//绘制大图编辑选区

    void drawSmallMapEditSelection(QPainter& painter, int startX, int startY, double cellWidth, double cellHeight);//绘制小图编辑选区

    void ensureMapTypeRow(const QString& value);//新bin不存在时补充表格行

    void updateTableCountsFromMapData();//按当前地图重新统计表格数量

protected:

    void mouseMoveEvent(QMouseEvent *e);
    void wheelEvent(QWheelEvent *event);

    virtual void mousePressEvent(QMouseEvent*) override;//重写鼠标点击
};
#endif // MAPTEST_H
