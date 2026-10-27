#ifndef QTWAFERMAP_WAFERMAPWINDOW_H
#define QTWAFERMAP_WAFERMAPWINDOW_H

#include <QColor>
#include <QHash>
#include <QImage>
#include <QMainWindow>
#include <QMap>
#include <QPoint>
#include <QRect>
#include <QRectF>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

#include <string>
#include <unordered_map>

#include "MapTypes.h"
#include "QTWaferMapGlobal.h"

class QLabel;
class QMouseEvent;
class QPainter;
class QResizeEvent;
class QTimer;
class QWheelEvent;
class RulerSlider;

QT_BEGIN_NAMESPACE
namespace Ui { class WaferMapWindow; }
QT_END_NAMESPACE

/**
 * @brief 交互式晶圆地图查看、编辑与路径规划窗口。
 *
 * WaferMapWindow 是一个自包含的 QMainWindow：
 * - 可以直接作为桌面程序的主窗口运行；
 * - 也可以作为其他 Qt 程序的主窗口嵌入使用。
 *
 * 坐标约定：
 * - 所有公开接口的坐标都是 0-based；
 * - 除非方法名或注释中明确写了“原始坐标（source）”，否则坐标指
 *   当前旋转方向下的显示坐标。
 *
 * 线程安全：这是 QWidget，只能在 GUI 线程中调用。
 *
 * 典型用法：
 * @code
 * WaferMapWindow window;
 * QString error;
 * if (!window.loadMapFile("wafer.txt", &error))
 *     qWarning() << error;
 * window.show();
 * @endcode
 */
class QTWAFERMAP_EXPORT WaferMapWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit WaferMapWindow(QWidget* parent = nullptr);
    ~WaferMapWindow() override;

    // ---------------------------------------------------------------------
    // 地图加载
    // ---------------------------------------------------------------------

    /**
     * @brief 自动识别格式和范围并加载地图。
     * @param filePath 地图文件路径。
     * @param errorMessage 可选的错误信息输出；成功时会被清空。
     * @return 加载到非空地图时返回 true。
     */
    bool loadMapFile(const QString& filePath, QString* errorMessage = nullptr);

    /**
     * @brief 使用内置加载模式加载地图。
     *
     * CustomLoadMode、CsvLoadMode 和 ExcelLoadMode 可能会弹出参数设置窗口。
     * 自动化、测试或嵌入场景请使用 MapLoadOptions 重载。
     */
    bool loadMapFile(const QString& filePath, MapLoadMode loadMode);

    /**
     * @brief 使用显式参数加载地图，全程不弹出任何对话框。
     *
     * 这是自动化、测试和嵌入调用推荐的加载入口。
     * 加载失败时返回 false，并写入 errorMessage / lastError()。
     */
    bool loadMapFile(const QString& filePath,
                     const MapLoadOptions& options,
                     QString* errorMessage = nullptr);

    /**
     * @brief 用内存中的二维 bin 数据替换当前地图。
     *
     * 行数据按显示顺序复制；空字符串和缺失的尾列会统一补成空格 bin。
     * 调用方不需要先写临时文件。
     */
    bool setMapData(const QVector<QStringList>& rows, QString* errorMessage = nullptr);

    /// 用扫码/条码原始文本在指定目录中查找地图文件，返回空字符串表示未找到。
    QString searchMapFileByScanText(const QString& scanText,
                                    const QStringList& searchRoots) const;

    /// 查找并加载地图；匹配到的文件路径可通过 matchedFilePath 返回。
    bool loadMapByScanText(const QString& scanText,
                           const QStringList& searchRoots,
                           MapLoadMode loadMode = DefaultLoadMode,
                           QString* matchedFilePath = nullptr);

    // ---------------------------------------------------------------------
    // 地图状态与数据查询
    // ---------------------------------------------------------------------

    /// 当前是否已经成功加载地图。
    bool hasMapLoaded() const;

    /// 当前地图文件路径；内存地图返回空字符串。
    QString currentMapFilePath() const;

    /// 最近一次加载失败的原因；成功后会被清空。
    QString lastError() const;

    int mapColumnCount() const;       ///< 原始地图列数（旋转前）。
    int mapRowCount() const;          ///< 原始地图行数（旋转前）。
    int displayColumnCount() const;   ///< 当前显示方向下的列数。
    int displayRowCount() const;      ///< 当前显示方向下的行数。

    /// 只读原始地图；下一次加载或编辑后引用可能失效。
    const WaferMapData& sourceMapData() const;

    /// 只读当前旋转方向下的地图，用于绘制。
    const WaferMapData& displayMapData() const;

    /// 读取原始地图指定坐标的 bin 值；越界返回空字符串。
    QString sourceCellValue(int column, int row) const;

    /// 读取当前显示方向指定坐标的 bin 值；越界返回空字符串。
    QString displayCellValue(int column, int row) const;

    /// 返回表格中的 bin 统计快照，顺序与界面表格一致。
    QVector<BinStatistic> binStatistics() const;

    // ---------------------------------------------------------------------
    // 实物坐标映射
    // ---------------------------------------------------------------------

    /// 设置原始行列到实物 X/Y 的线性映射，并刷新地图信息中的坐标显示。
    void setCoordinateMapping(const MapCoordinateMapping& mapping);

    /// 当前的实物坐标映射配置。
    MapCoordinateMapping coordinateMapping() const;

    /// 设置外部绑定的精确实物坐标数据，按 sourceCell 保存，支持不规则坐标。
    void setPhysicalCoordinateData(const QVector<WaferMapPhysicalPoint>& points);

    /// 返回当前绑定的精确实物坐标数据。
    QVector<WaferMapPhysicalPoint> physicalCoordinateData() const;

    /// 清空外部绑定的精确实物坐标数据；线性映射配置不会被清除。
    void clearPhysicalCoordinateData();

    /// 把原始地图坐标（0-based）换算成实物坐标。
    QPointF sourceToPhysical(int column, int row) const;

    /// 当前选中格的实物坐标；内部会自动把显示坐标换算成原始坐标。
    QPointF currentPhysicalCoordinate() const;

    /// 当前蛇形行进方向。
    Direction mapDirection() const;

    /// 设置行进方向；自动运行期间返回 false。
    bool setMapDirection(Direction direction);

    /// 当前显示旋转角度（0、90、180 或 270）。
    int mapRotation() const;

    /// 设置显示旋转角度；degrees 必须是 90 的倍数。
    bool setMapRotation(int degrees);

    // ---------------------------------------------------------------------
    // 当前选中格与对点
    // ---------------------------------------------------------------------

    /// 当前选中的显示坐标。
    QPoint currentDisplayCell() const;

    /// 选中指定显示坐标；centerView 控制是否把视图中心移过去。
    bool selectDisplayCell(int column, int row, bool centerView = true, bool refresh = true);

    /// 把指定显示坐标设置为起点，内部保存为原始坐标。
    bool setStartPoint(int column, int row);

    /// 把指定显示坐标设置为唯一参考点，内部保存为原始坐标。
    bool setReferencePoint(int column, int row);

    /// 把指定显示坐标追加为验证点，内部保存为原始坐标。
    bool addVerifyPoint(int column, int row);

    /// 清空所有验证点。
    void clearVerifyPointList();

    /// 是否已经设置起点。
    bool hasStartPoint() const;

    /// 是否已经设置参考点。
    bool hasReferencePoint() const;

    QPoint startPoint() const;              ///< 起点，原始坐标。
    QPoint referencePoint() const;          ///< 参考点，原始坐标。
    QVector<QPoint> verifyPoints() const;   ///< 验证点列表，原始坐标。

    // ---------------------------------------------------------------------
    // 寻起点
    // ---------------------------------------------------------------------

    /// 计算“参考点 -> 验证点 -> 起点”的寻起点路径，但不启动自动步进。
    bool prepareFindStartRoute();

    /// 开始自动寻起点，内部定时器会按固定间隔前进一步。
    bool startFindStartPoint();

    /// 停止寻起点；reason 会显示在状态栏。
    void stopFindStartPoint(const QString& reason = QString());

    /// 读取寻起点的下一步显示坐标，不移动界面。
    bool nextFindStartDisplayCell(QPoint* displayCell);

    /// 读取寻起点的下一步原始坐标，不移动界面。
    bool nextFindStartSourceCell(QPoint* sourceCell);

    /// 返回寻起点剩余路径（显示坐标）。
    QVector<QPoint> pendingFindStartPathDisplayCells() const;

    /// 返回寻起点剩余路径（原始坐标）。
    QVector<QPoint> pendingFindStartPathSourceCells() const;

    /// 寻起点前进一步并刷新界面。
    bool moveToNextFindStartCell(QPoint* movedDisplayCell = nullptr,
                                 bool* reachedStart = nullptr);

    /// 开始蛇形取料路径规划。
    bool startMapPathPlanning();

    /// 停止寻起点或路径规划。
    void stopMapPathPlanning();

    /// 是否已经按参考点和验证点走到起点。
    bool hasFoundStartPoint() const;

    /// 是否正在执行寻起点步进。
    bool isFindingStartPoint() const;

    /// 是否正在自动运行（寻起点或路径规划）。
    bool isPathPlanningRunning() const;

    // ---------------------------------------------------------------------
    // 取料路径规划
    // ---------------------------------------------------------------------

    /// 把当前格标记为已吸取并刷新界面。
    bool markCurrentCellPicked();

    /// 选中指定显示坐标并标记为已吸取。
    bool markDisplayCellPicked(int column, int row);

    /// 计算从当前位置到下一颗吸取点的路径。
    bool prepareNextPickupRoute();

    /// 读取下一颗吸取点的显示坐标。
    bool nextPickupDisplayCell(QPoint* displayCell);

    /// 读取下一颗吸取点的原始坐标。
    bool nextPickupSourceCell(QPoint* sourceCell);

    /// 到下一颗吸取点的显示坐标路径，最后一个点是吸取点。
    QVector<QPoint> nextMovePathDisplayCells();

    /// 到下一颗吸取点的原始坐标路径，最后一个点是吸取点。
    QVector<QPoint> nextMovePathSourceCells();

    /// 当前绕路/借助路径的显示坐标，不包含最后的吸取点。
    QVector<QPoint> nextAssistPathDisplayCells();

    /// 当前绕路/借助路径的原始坐标，不包含最后的吸取点。
    QVector<QPoint> nextAssistPathSourceCells();

    /// 沿待走路径前进一步并刷新界面。
    bool moveToNextPlannedCell(QPoint* movedDisplayCell = nullptr,
                               bool* isPickupCell = nullptr);

    /// 当前尚未行走的显示坐标路径。
    QVector<QPoint> pendingMovePathDisplayCells() const;

    /// 当前尚未行走的原始坐标路径。
    QVector<QPoint> pendingMovePathSourceCells() const;

signals:
    /// 文件或内存地图加载成功后发送；内存地图的 filePath 为空。
    void mapLoaded(const QString& filePath);

    /// 加载失败时发送；错误信息同时可通过 lastError() 获取。
    void mapLoadFailed(const QString& errorMessage);

    /// 当前选中的显示格发生变化时发送。
    void currentCellChanged(int column, int row);

    /// 行进方向改变后发送。
    void directionChanged(Direction direction);

    /// 显示旋转角度改变后发送。
    void rotationChanged(int degrees);

    /// 编辑操作修改地图单元格后发送。
    void mapEdited();

private:
    // ===== 界面与视图 =====
    Ui::WaferMapWindow* ui;                 ///< uic 生成的界面对象。

    double m_scaleFactor = 1.0;             ///< 大图当前缩放因子。
    const double m_minScaleFactor = 1.0;    ///< 大图最小缩放倍率。
    const double m_maxScaleFactor = 16.0;   ///< 大图最大缩放倍率。
    QTimer* m_pTimer = nullptr;             ///< 寻起点和路径规划共用的步进定时器。
    QImage m_mapColorImage;                 ///< 当前旋转方向下的一格一像素缓存图。
    bool m_mapColorImageDirty = true;       ///< 地图内容或颜色变化后需要重建缓存图。

    RulerSlider* BigMapHorizontalslider = nullptr;  ///< 大图水平刻度尺。
    RulerSlider* BigMapVerticalslider = nullptr;    ///< 大图垂直刻度尺。
    RulerSlider* SmallMapHorizontalslider = nullptr;///< 小图水平刻度尺。
    RulerSlider* SmallMapVerticalslider = nullptr;  ///< 小图垂直刻度尺。

    QPoint m_dragStartPosition;             ///< 大图拖拽开始时的鼠标位置。
    QPoint m_lastDragPosition;              ///< 大图拖拽上一次鼠标位置。
    QPointF m_viewCenterCell;               ///< 当前大图视图中心所在的显示坐标。
    bool m_isDragging = false;              ///< 是否正在拖拽平移大图。
    bool m_dragMoved = false;               ///< 本次鼠标按下后是否发生过拖动。
    int RecordCurrent_x = 0;                ///< 当前选中格的显示列号。
    int RecordCurrent_y = 0;                ///< 当前选中格的显示行号。
    int map_columns = 0;                    ///< 原始地图列数。
    int map_rows = 0;                       ///< 原始地图行数。
    int m_rotation = 0;                     ///< 显示旋转角度（0/90/180/270）。

    // ===== 地图数据 =====
    QString m_mapFilePath;                  ///< 当前地图文件路径；内存地图为空。
    WaferMapData m_mapData;                 ///< 原始地图数据。
    WaferMapData rotate_m_mapData;          ///< 按当前旋转角度生成的显示地图。
    MapCoordinateMapping m_coordinateMapping; ///< 原始行列到实物 X/Y 的映射配置。
    QVector<WaferMapPhysicalPoint> m_physicalCoordinateData; ///< 外部绑定的精确实物坐标。
    QHash<qint64, QPointF> m_physicalCoordinateLookup; ///< 精确坐标按原始坐标 key 建立的快速查找表。

    // ===== 方向与对点 =====
    Direction m_selectedDirection = LeftToRightTopToBottom; ///< 当前蛇形行进方向。
    int m_currentMinorDx = 0;               ///< 当前次方向 X，蛇形换行/换列后更新。
    int m_currentMinorDy = 0;               ///< 当前次方向 Y，蛇形换行/换列后更新。
    bool m_hasStartPoint = false;           ///< 是否已经设置起点。
    QPoint m_startPointSource;              ///< 起点，原始地图坐标。
    bool m_hasReferencePoint = false;       ///< 是否已经设置参考点。
    QPoint m_referencePointSource;          ///< 参考点，原始地图坐标。
    QVector<QPoint> m_verifyPointsSource;   ///< 验证点列表，原始地图坐标。
    bool m_hasFoundStartPoint = false;      ///< 是否已经按参考点和验证点寻到起点。
    bool m_isFindingStartPoint = false;     ///< 是否正在执行寻起点步进。

    // ===== 地图编辑 =====
    MapEditSelectionMode m_editSelectionMode = MapEditPoint; ///< 当前编辑选区模式。
    bool m_hasEditRectStart = false;        ///< 框选左上点是否已设置。
    bool m_hasEditRectEnd = false;          ///< 框选右下点是否已设置。
    QPoint m_editRectStartDisplay;          ///< 框选左上点，显示坐标。
    QPoint m_editRectEndDisplay;            ///< 框选右下点，显示坐标。

    // ===== 加载参数 =====
    bool m_hasLoadRange = false;            ///< 是否使用手动/确认后的加载范围。
    int m_loadStartRow = 1;                 ///< 加载起始行（1-based，内部使用）。
    int m_loadEndRow = 1;                   ///< 加载结束行（1-based，内部使用）。
    int m_loadStartColumn = 1;              ///< 加载起始列（1-based，内部使用）。
    int m_loadEndColumn = 1;                ///< 加载结束列（1-based，内部使用）。
    int m_loadBinCharCount = 1;             ///< 自定义加载时每个 bin 占用的字符数。
    int m_activeLoadMode = 0;               ///< 当前加载方式。
    int m_csvDataStartRow = 1;              ///< CSV 数据起始行（0-based）。
    int m_csvBinColumn = 2;                 ///< CSV bin 列（0-based）。
    int m_csvXColumn = 0;                   ///< CSV X 坐标列（0-based）。
    int m_csvYColumn = 1;                   ///< CSV Y 坐标列（0-based）。
    int m_excelPageIndex = 0;               ///< Excel 工作表索引（0-based）。
    QString m_lastScanSearchPath;           ///< 手动扫码上次选择的搜索目录。

    // ===== 路径与加工状态 =====
    QVector<QPoint> m_pendingPathDisplayCells; ///< 启动后待走的显示坐标路径。
    QString m_processedCellText = QStringLiteral("#"); ///< 已加工写入地图的单字符。
    QSet<qint64> m_processedSourceCells;    ///< 已加工点，保存原始地图坐标 key。
    QMap<qint64, std::string> m_processedOriginalValues; ///< 已加工点被写入符号前的原始 bin 值。
    QSet<qint64> m_assistPathSourceCells;   ///< 借助路径，保存原始地图坐标 key。
    QVector<QPoint> m_pendingFindStartDisplayCells; ///< 寻起点待走的显示坐标路径。
    QSet<qint64> m_findStartPathSourceCells;///< 已经走过的寻起点路线，原始坐标 key。

    // ===== 布局与状态 =====
    bool m_isSyncingMapWidgetGeometry = false; ///< 是否正在同步模块布局，避免 Resize 递归。
    bool m_hasSmallMapChildLayoutBase = false; ///< 是否已记录小图模块内控件的 UI 设计尺寸。
    QRect m_baseTableWidgetRect;            ///< UI 设计时表格区域的位置尺寸。
    QRect m_baseSmallLabelRect;             ///< UI 设计时小图区域的位置尺寸。

    bool m_showLoadDialogs = true;          ///< 加载失败时是否允许弹出提示框。
    QString m_lastError;                    ///< 最近一次加载错误。

private slots:
    void handleTimeout();                   ///< 寻起点/路径规划的定时步进。
    void on_read_pushButton_clicked();      ///< “加载文档”按钮。
    void on_rotate_clicked();               ///< “旋转Map图”按钮。
    void on_tableWidget_cellClicked(int row, int column); ///< bin 表格点击。
    void on_start_pushButton_clicked();     ///< 开始/停止路径规划。
    void on_direction_pushButton_clicked(); ///< 弹出行进方向选择窗口。

private:
    // ===== 加载 =====
    bool loadMapFileFromPath(const QString& filePath,
                             int loadMode,
                             bool interactive,
                             QString* errorMessage); ///< 地图加载统一入口。
    bool readMapTextFile();                  ///< 读取普通文本或 .map 地图。
    bool configureMapLoadRange(const QString& filePath); ///< 文本范围设置窗口。
    bool configureCsvLoadOptions(const QString& filePath); ///< CSV 列设置窗口。
    bool configureExcelLoadRange(const QString& filePath); ///< Excel 范围设置窗口。
    bool loadCsvMapTxtFile();                ///< 按 CSV 列配置读取文件。
    bool loadExcelMapFile();                 ///< 按 Excel 单元格范围读取文件。
    void finishLoadedMapData();              ///< 加载完成后统一刷新表格、统计和视图。
    QString findMapFileByScanText(const QString& scanText, const QStringList& searchRoots) const; ///< 扫码查找实现。
    void reportLoadError(const QString& title, const QString& message); ///< 交互式加载错误提示。
    bool setLoadError(const QString& message); ///< 记录加载错误。

    // ===== 事件与视图 =====
    bool eventFilter(QObject* watched, QEvent* event) override; ///< 地图控件事件过滤器。
    void resetMapScale();                    ///< 恢复原始视图。
    void DrawSmallMap(QLabel* targetLabel,
                      int centerX,
                      int centerY,
                      const std::unordered_map<std::string, QColor>& colorMap); ///< 绘制局部小图。
    int displayColumns() const;              ///< 当前显示方向下的列数。
    int displayRows() const;                 ///< 当前显示方向下的行数。
    const WaferMapData& currentMapData() const; ///< 按当前旋转角度返回绘图用地图。
    std::unordered_map<std::string, QColor> buildColorMap() const; ///< 从表格读取 bin 颜色映射。
    std::unordered_map<std::string, int> checkedTargetRows() const; ///< 读取勾选的目标 bin。
    bool processCurrentCellIfChecked();      ///< 当前格是加工目标时标记为已加工。
    QString chooseProcessedCellText(const std::unordered_map<std::string, int>& stringCounts) const; ///< 选择不冲突的已加工字符。
    bool isProcessedTableRow(int row) const; ///< 判断表格行是否为已加工统计行。
    bool updateProcessedCellText(const QString& text, int row); ///< 修改已加工字符并同步地图。
    QRectF currentViewRect() const;          ///< 大图当前可见的地图坐标范围。
    QPoint cellFromMainPoint(const QPoint& pos) const; ///< 大图像素坐标转显示地图坐标。
    QPoint cellFromSmallPoint(const QPoint& pos) const; ///< 小图像素坐标转显示地图坐标。
    QPoint displayToSourceCell(int x, int y) const; ///< 显示坐标转原始地图坐标。
    QPoint sourceToDisplayCell(int x, int y) const; ///< 原始地图坐标转显示坐标。
    int clampToRange(int value, int minValue, int maxValue) const; ///< 整数范围限制。
    double clampToRange(double value, double minValue, double maxValue) const; ///< 浮点范围限制。
    qint64 sourceCellKey(int x, int y) const; ///< 原始坐标生成唯一 key。

    // ===== 路径标记 =====
    bool isAssistPathDisplayCell(int displayX, int displayY) const; ///< 是否为借助路径格。
    bool isFindStartPathDisplayCell(int displayX, int displayY) const; ///< 是否为寻起点路线格。
    void markAssistPathCell(int displayX, int displayY); ///< 标记灰色借助路径。
    void markFindStartPathCell(int displayX, int displayY); ///< 标记橙色寻起点路线。
    void clearFindStartRoute();              ///< 清空寻起点路线。
    void setSelectedCell(int x, int y, bool centerView, bool refresh = true); ///< 更新当前选中格。
    void centerViewOnCell(int x, int y);     ///< 把视图中心移动到指定格。
    void zoomMainMap(const QPoint& anchor, double factor); ///< 按鼠标锚点缩放。
    void panMainMap(const QPoint& pixelDelta); ///< 拖拽平移大图。
    void renderMainMap();                    ///< 重绘大图。
    void renderSmallMap(const std::unordered_map<std::string, QColor>& colorMap); ///< 重绘小图。
    void refreshMapViews();                  ///< 刷新大小图、坐标和刻度。

    // ===== 界面刷新 =====
    void updateSelectionUi();                ///< 刷新文件/行列/角度/实物坐标信息。
    void updatePointAlignmentUi();           ///< 刷新对点坐标显示。
    void updateRulers();                     ///< 刷新刻度尺。
    void setupMapTableView();                ///< 初始化表格样式。
    void updateTableColumnWidths();          ///< 按表格宽度调整列宽。
    void captureSmallMapChildLayoutBase();   ///< 记录小图模块的 UI 设计尺寸。
    void syncMapWidgetGeometry();            ///< 同步刻度尺位置和地图控件尺寸。
    void rebuildRotatedMap();                ///< 按旋转角度重建显示地图。
    void invalidateMapColorImage();          ///< 标记地图缓存图失效。
    void rebuildMapColorImage(const std::unordered_map<std::string, QColor>& colorMap); ///< 重建缓存图。
    bool updateMapColorImagePixel(int displayX, int displayY, const QColor& color); ///< 局部更新缓存像素。
    bool advanceSearchStep();                ///< 推进一步加工/借助路径。
    bool advanceFindStartStep();             ///< 推进一步寻起点。
    void updateDirectionButton();            ///< 刷新方向按钮文字和图标。
    void setupMapFunctionPanel();            ///< 初始化右侧 Map 功能区信号。
    void setMapFunctionPanelRunning(bool running); ///< 锁定额外控件。

    // ===== 对点与编辑 =====
    void setStartPointFromSelection();       ///< 当前格设为起点。
    void setReferencePointFromSelection();   ///< 当前格设为参考点。
    void addVerifyPointFromSelection();      ///< 当前格加入验证点。
    void clearVerifyPoints();                ///< 清空验证点。
    void findStartPointRoute();              ///< 按参考点/验证点/起点生成路线。
    void drawMainMapMarkers(QPainter& painter, const QRectF& view, double cellWidth, double cellHeight); ///< 绘制大图点位标记。
    void drawSmallMapMarkers(QPainter& painter, int startX, int startY, double cellWidth, double cellHeight, int cellCount); ///< 绘制小图点位标记。
    void updateEditSelectionUi();            ///< 刷新编辑选区坐标。
    void setEditSelectionMode(MapEditSelectionMode mode); ///< 切换点选/框选/圆选。
    void setEditRectStartFromSelection();    ///< 当前格设为框选左上点。
    void setEditRectEndFromSelection();      ///< 当前格设为框选右下点。
    QVector<QPoint> editSelectionSourceCells() const; ///< 获取编辑选区对应的原始坐标。
    void applyEditToSelection(const QString& newValue); ///< 修改或裁剪当前选区。
    void drawMainMapEditSelection(QPainter& painter, const QRectF& view, double cellWidth, double cellHeight); ///< 绘制大图编辑选区。
    void drawSmallMapEditSelection(QPainter& painter, int startX, int startY, double cellWidth, double cellHeight); ///< 绘制小图编辑选区。
    void ensureMapTypeRow(const QString& value); ///< 新 bin 不存在时补充表格行。
    void updateTableCountsFromMapData();     ///< 按地图内容重新统计表格数量。

protected:
    void wheelEvent(QWheelEvent* event) override; ///< 鼠标滚轮缩放大图。
    void resizeEvent(QResizeEvent* event) override; ///< 窗口尺寸变化时同步地图控件。
};

#endif // QTWAFERMAP_WAFERMAPWINDOW_H
