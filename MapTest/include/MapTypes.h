#ifndef QTWAFERMAP_MAPTYPES_H
#define QTWAFERMAP_MAPTYPES_H

#include <QColor>
#include <QMap>
#include <QMetaType>
#include <QPoint>
#include <QPointF>
#include <QString>

#include <string>

/**
 * @brief 行优先的晶圆地图数据。
 *
 * 外层 key 是原始行号（0-based），内层 key 是原始列号（0-based）。
 * value 使用 std::string，兼容单字符 bin（如 "1"）和多字符 bin（如 "01"）。
 * 缺失坐标或空白区域统一用空格 " " 表示。
 */
using WaferMapData = QMap<int, QMap<int, std::string>>;

/**
 * @brief 蛇形行进方向。
 *
 * 前半部分是行/列内的快速扫描方向，后半部分是从当前行/列切换到下一行/列的方向。
 * 例如 LeftToRightTopToBottom 表示：先从左到右扫完一行，再整体从上到下换行。
 */
enum Direction
{
    LeftToRightTopToBottom,    ///< 左到右扫描，整体从上到下换行。
    LeftToRightBottomToTop,    ///< 左到右扫描，整体从下到上换行。
    RightToLeftTopToBottom,    ///< 右到左扫描，整体从上到下换行。
    RightToLeftBottomToTop,    ///< 右到左扫描，整体从下到上换行。
    TopToBottomLeftToRight,    ///< 上到下扫描，整体从左到右换列。
    TopToBottomRightToLeft,    ///< 上到下扫描，整体从右到左换列。
    BottomToTopLeftToRight,    ///< 下到上扫描，整体从左到右换列。
    BottomToTopRightToLeft     ///< 下到上扫描，整体从右到左换列。
};

/// 地图编辑时的选区形状。
enum MapEditSelectionMode
{
    MapEditPoint,  ///< 点选：只编辑当前格。
    MapEditRect,   ///< 框选：由两个对角点组成矩形选区。
    MapEditCircle  ///< 圆选：以当前格为圆心、按半径生成选区。
};

/// 地图文件的加载方式。
enum MapLoadMode
{
    DefaultLoadMode = 0, ///< 自动识别行列范围和 bin 宽度。
    CsvLoadMode = 1,     ///< 按 CSV 的 X/Y/bin 列读取。
    CustomLoadMode = 2,  ///< 使用显式指定的文本行列范围。
    ExcelLoadMode = 3    ///< 读取 Office Open XML 工作表范围。
};

/**
 * @brief 地图加载范围（闭区间，0-based）。
 *
 * 四个字段都保持 -1 时表示由加载器自动检测。
 * 只设置部分字段会被视为无效范围，加载接口会返回明确的错误。
 */
struct MapRange
{
    int startRow = -1;      ///< 起始行（含），0-based。
    int endRow = -1;        ///< 结束行（含），0-based。
    int startColumn = -1;   ///< 起始列（含），0-based。
    int endColumn = -1;     ///< 结束列（含），0-based。

    /// 是否完全未设置；未设置时由加载器自动检测。
    bool isEmpty() const
    {
        return startRow < 0 && endRow < 0 && startColumn < 0 && endColumn < 0;
    }

    /// 是否为有效的闭区间；结束行列必须大于等于起始行列。
    bool isValid() const
    {
        return startRow >= 0 && endRow >= startRow
                && startColumn >= 0 && endColumn >= startColumn;
    }
};

/**
 * @brief WaferMapWindow::loadMapFile() 的显式加载参数。
 *
 * 只使用与当前模式相关的字段：
 * - DefaultLoadMode：range 可选；有效时按指定范围加载。
 * - CustomLoadMode：必须提供有效 range，binCharCount 控制 bin 宽度。
 * - CsvLoadMode：csv* 字段指定数据起始行和 X/Y/bin 列。
 * - ExcelLoadMode：excelSheetIndex 和 range 指定工作表和单元格范围。
 *
 * .xlsx/.xlsm/.xltx 文件始终走 Excel 读取器，即使调用方指定了其他模式。
 */
struct MapLoadOptions
{
    MapLoadMode mode = DefaultLoadMode; ///< 加载模式。
    MapRange range;                     ///< 文本/Excel 的可选行列范围。
    int binCharCount = 1;               ///< 文本/自定义模式下每个 bin 占用的字符数，范围 1..4。
    int csvDataStartRow = 1;            ///< CSV 数据起始行（0-based），默认 1 表示跳过一行表头。
    int csvXColumn = 0;                 ///< CSV X 坐标列（0-based）。
    int csvYColumn = 1;                 ///< CSV Y 坐标列（0-based）。
    int csvBinColumn = 2;               ///< CSV bin 值列（0-based）。
    int excelSheetIndex = 0;            ///< Excel 工作表索引（0-based）。
};

/// bin 统计表中的一行，对应 WaferMapWindow::binStatistics() 的返回元素。
struct BinStatistic
{
    QString bin;        ///< bin 值，与界面表格显示一致。
    int total = 0;      ///< 该 bin 的总数量。
    int processed = 0;  ///< 已标记为加工的数量。
    int remaining = 0;  ///< 尚未加工的数量。
    QColor color;       ///< 当前渲染使用的颜色。
};

/**
 * @brief 网格坐标到实物坐标的线性映射。
 *
 * 计算公式：
 * - physicalX = originX + sourceColumn * stepX
 * - physicalY = originY + sourceRow * stepY
 *
 * sourceColumn/sourceRow 是原始地图的 0-based 行列号，和显示旋转无关。
 * 默认禁用；启用后主界面地图信息会显示当前选中格的 X/Y 实物坐标。
 * unit 只用于界面显示，可以是 mm、um、die 或其他单位。
 */
struct MapCoordinateMapping
{
    bool enabled = false;       ///< 是否启用并在地图信息中显示 X/Y。
    double originX = 0.0;       ///< 原始列号为 0 时的 X 坐标。
    double originY = 0.0;       ///< 原始行号为 0 时的 Y 坐标。
    double stepX = 1.0;         ///< 每增加一列，X 坐标的增量。
    double stepY = 1.0;         ///< 每增加一行，Y 坐标的增量；向上为 Y 正方向时可设为负数。
    QString unit = QStringLiteral("mm"); ///< 显示单位。
};

/**
 * @brief 一个外部绑定的精确实物坐标点。
 *
 * sourceCell 使用原始地图的 0-based 坐标，不受显示旋转影响。
 * physical 保存设备或工艺系统中的实际 X/Y 坐标。
 */
struct WaferMapPhysicalPoint
{
    QPoint sourceCell;   ///< 原始地图坐标（0-based）。
    QPointF physical;    ///< 实物 X/Y 坐标。
};

Q_DECLARE_METATYPE(Direction)

#endif // QTWAFERMAP_MAPTYPES_H
