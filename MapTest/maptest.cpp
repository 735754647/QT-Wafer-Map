#include "maptest.h"
#include "ui_maptest.h"
#include "rulerslider.h"
#include <queue>
#include <algorithm>
#include <unordered_set>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QFileInfo>
#include <QAbstractButton>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QRadioButton>
#include <QRandomGenerator>
#include <QRegularExpressionValidator>
#include <QResizeEvent>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStyle>
#include <QStyleFactory>
#include <QVBoxLayout>
#include <QXmlStreamReader>
#include <QtGui/private/qzipreader_p.h>
#include <climits>
#include <cmath>

namespace
{
using MapData = QMap<int, QMap<int, std::string>>;

const int MaxLoadBinCharCount = 4;//自定义加载时单个bin最多允许占用的字符数
const int RulerThickness = 35;//大小地图刻度尺控件预留宽度/高度
const int ModuleVerticalGap = 20;//小图、表格等模块之间的垂直间距
const int StatusVerticalGap = 15;//地图状态文字和地图模块之间的垂直间距
const int MapStepIntervalMs = 100;//寻起点/路径规划自动步进间隔，单位ms
const QString ProcessedCellText = QStringLiteral("已加工");//表格中已加工统计行的显示名称
const QColor ProcessedCellColor = Qt::white;//已加工符号颜色无效时使用的默认颜色
const std::string BlankCellValue = " ";//地图缺失坐标或空白区域统一按空格bin处理

// QMap::value(row) 会复制整行，路径搜索里必须用 constFind 直接取引用。
const std::string& mapCellValue(const MapData& mapData, int row, int col)
{
    const auto rowIter = mapData.constFind(row);
    if (rowIter == mapData.constEnd())
    {
        return BlankCellValue;
    }

    const auto cellIter = rowIter.value().constFind(col);
    return cellIter == rowIter.value().constEnd() ? BlankCellValue : cellIter.value();
}

// 将方向枚举转换成按钮、弹窗中显示的中文说明。
QString directionText(Direction direction)
{
    switch (direction)
    {
    case LeftToRightTopToBottom:
        return QStringLiteral("左→右,上→下");
    case LeftToRightBottomToTop:
        return QStringLiteral("左→右,下→上");
    case RightToLeftTopToBottom:
        return QStringLiteral("右→左,上→下");
    case RightToLeftBottomToTop:
        return QStringLiteral("右→左,下→上");
    case TopToBottomLeftToRight:
        return QStringLiteral("上→下,左→右");
    case TopToBottomRightToLeft:
        return QStringLiteral("上→下,右→左");
    case BottomToTopLeftToRight:
        return QStringLiteral("下→上,左→右");
    case BottomToTopRightToLeft:
        return QStringLiteral("下→上,右→左");
    }
    return QStringLiteral("左→右,上→下");
}

// 在方向图标里绘制一段带箭头的线。
void drawArrow(QPainter& painter, const QPointF& start, const QPointF& end)
{
    painter.drawLine(start, end);

    const double pi = 3.14159265358979323846;
    const double angle = std::atan2(end.y() - start.y(), end.x() - start.x());
    const double arrowSize = 7.0;
    QPointF arrowP1(end.x() - arrowSize * std::cos(angle - pi / 6.0),
                    end.y() - arrowSize * std::sin(angle - pi / 6.0));
    QPointF arrowP2(end.x() - arrowSize * std::cos(angle + pi / 6.0),
                    end.y() - arrowSize * std::sin(angle + pi / 6.0));

    QPolygonF arrowHead;
    arrowHead << end << arrowP1 << arrowP2;
    painter.save();
    painter.setBrush(painter.pen().color());
    painter.drawPolygon(arrowHead);
    painter.restore();
}

// 根据行进方向生成示意图标，方便直观看出蛇形走向。
QIcon directionIcon(Direction direction)
{
    QPixmap pixmap(74, 48);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(Qt::black, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    const QPointF leftTop(11, 12);
    const QPointF rightTop(63, 12);
    const QPointF leftBottom(11, 36);
    const QPointF rightBottom(63, 36);

    switch (direction)
    {
    case LeftToRightTopToBottom:
        drawArrow(painter, leftTop, rightTop);
        painter.drawLine(rightTop, rightBottom);
        drawArrow(painter, rightBottom, leftBottom);
        break;
    case LeftToRightBottomToTop:
        drawArrow(painter, leftBottom, rightBottom);
        painter.drawLine(rightBottom, rightTop);
        drawArrow(painter, rightTop, leftTop);
        break;
    case RightToLeftTopToBottom:
        drawArrow(painter, rightTop, leftTop);
        painter.drawLine(leftTop, leftBottom);
        drawArrow(painter, leftBottom, rightBottom);
        break;
    case RightToLeftBottomToTop:
        drawArrow(painter, rightBottom, leftBottom);
        painter.drawLine(leftBottom, leftTop);
        drawArrow(painter, leftTop, rightTop);
        break;
    case TopToBottomLeftToRight:
        drawArrow(painter, leftTop, leftBottom);
        painter.drawLine(leftBottom, rightBottom);
        drawArrow(painter, rightBottom, rightTop);
        break;
    case TopToBottomRightToLeft:
        drawArrow(painter, rightTop, rightBottom);
        painter.drawLine(rightBottom, leftBottom);
        drawArrow(painter, leftBottom, leftTop);
        break;
    case BottomToTopLeftToRight:
        drawArrow(painter, leftBottom, leftTop);
        painter.drawLine(leftTop, rightTop);
        drawArrow(painter, rightTop, rightBottom);
        break;
    case BottomToTopRightToLeft:
        drawArrow(painter, rightBottom, rightTop);
        painter.drawLine(rightTop, leftTop);
        drawArrow(painter, leftTop, leftBottom);
        break;
    }

    return QIcon(pixmap);
}

// 单步方向向量，dx/dy取-1、0、1，表示列/行移动方向。
struct DirectionVector
{
    int dx;//列方向增量
    int dy;//行方向增量
};

// 一种行进方向拆成主方向和次方向，主方向用于换行/换列，次方向用于当前行/列内扫描。
struct MapDirectionVectors
{
    DirectionVector major;//主扫描方向
    DirectionVector minor;//当前行/列内扫描方向
};

// 返回方向向量的反向，用于蛇形换行或换列。
DirectionVector negateVector(const DirectionVector& vector)
{
    return {-vector.dx, -vector.dy};
}

// 判断方向向量是否沿水平方向移动。
bool isHorizontal(const DirectionVector& vector)
{
    return vector.dx != 0 && vector.dy == 0;
}

// 判断方向向量是否沿垂直方向移动。
bool isVertical(const DirectionVector& vector)
{
    return vector.dx == 0 && vector.dy != 0;
}

// 判断两个方向是否属于同一轴，避免当前次方向和选定方向冲突。
bool isSameAxis(const DirectionVector& left, const DirectionVector& right)
{
    return (isHorizontal(left) && isHorizontal(right)) || (isVertical(left) && isVertical(right));
}

// 把用户选择的方向拆成主扫描方向和行/列内的次扫描方向。
MapDirectionVectors directionVectors(Direction direction)
{
    switch (direction)
    {
    case LeftToRightTopToBottom:
        return {{0, 1}, {1, 0}};
    case LeftToRightBottomToTop:
        return {{0, -1}, {1, 0}};
    case RightToLeftTopToBottom:
        return {{0, 1}, {-1, 0}};
    case RightToLeftBottomToTop:
        return {{0, -1}, {-1, 0}};
    case TopToBottomLeftToRight:
        return {{1, 0}, {0, 1}};
    case TopToBottomRightToLeft:
        return {{-1, 0}, {0, 1}};
    case BottomToTopLeftToRight:
        return {{1, 0}, {0, -1}};
    case BottomToTopRightToLeft:
        return {{-1, 0}, {0, -1}};
    }
    return {{0, 1}, {1, 0}};
}

// 判断指定地图坐标是否属于目标bin集合。
bool containsTarget(const MapData& mapData,
                    const std::unordered_set<std::string>& targetSet,
                    int row,
                    int col)
{
    return targetSet.find(mapCellValue(mapData, row, col)) != targetSet.end();
}

// 判断指定地图坐标是否属于目标bin列表。
bool containsTarget(const MapData& mapData,
                    const std::vector<std::string>& targetChars,
                    int row,
                    int col)
{
    const std::string& value = mapCellValue(mapData, row, col);
    return std::find(targetChars.begin(), targetChars.end(), value) != targetChars.end();
}

// 按当前行进方向从地图起始边界查找第一个需要加工的点。
std::pair<int, int> findFirstTargetPosition(const MapData& mapData,
                                            const std::vector<std::string>& targetChars,
                                            Direction direction)
{
    const int rows = mapData.size();
    if (rows <= 0)
    {
        return {-1, -1};
    }

    const int cols = mapData.constBegin().value().size();
    if (cols <= 0 || targetChars.empty())
    {
        return {-1, -1};
    }

    MapDirectionVectors vectors = directionVectors(direction);
    std::unordered_set<std::string> targetSet(targetChars.begin(), targetChars.end());

    if (isVertical(vectors.major))
    {
        for (int row = vectors.major.dy > 0 ? 0 : rows - 1;
             row >= 0 && row < rows;
             row += vectors.major.dy)
        {
            for (int col = vectors.minor.dx > 0 ? 0 : cols - 1;
                 col >= 0 && col < cols;
                 col += vectors.minor.dx)
            {
                if (containsTarget(mapData, targetSet, row, col))
                {
                    return {row, col};
                }
            }
        }
    }
    else
    {
        for (int col = vectors.major.dx > 0 ? 0 : cols - 1;
             col >= 0 && col < cols;
             col += vectors.major.dx)
        {
            for (int row = vectors.minor.dy > 0 ? 0 : rows - 1;
                 row >= 0 && row < rows;
                 row += vectors.minor.dy)
            {
                if (containsTarget(mapData, targetSet, row, col))
                {
                    return {row, col};
                }
            }
        }
    }

    return {-1, -1};
}

// 生成起点五角星标记的顶点。
QPolygonF starPolygon(const QPointF& center, double outerRadius)
{
    QPolygonF polygon;
    const double innerRadius = outerRadius * 0.45;
    const double pi = 3.14159265358979323846;
    for (int i = 0; i < 10; ++i)
    {
        double radius = (i % 2 == 0) ? outerRadius : innerRadius;
        double angle = -pi / 2.0 + i * pi / 5.0;
        polygon << QPointF(center.x() + std::cos(angle) * radius,
                           center.y() + std::sin(angle) * radius);
    }
    return polygon;
}

// 在地图上绘制起点五角星标记。
void drawStartMarker(QPainter& painter, const QPointF& center, double radius)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(Qt::red, 2));
    painter.setBrush(QColor(255, 230, 0));
    painter.drawPolygon(starPolygon(center, radius));
    painter.restore();
}

// 绘制参考/验证点倒矩形标记，并在内部显示标识。
void drawReferenceMarker(QPainter& painter,
                         const QPointF& tip,
                         double size,
                         const QString& text,
                         const QColor& borderColor,
                         const QColor& fillColor)
{
    const double halfWidth = size * 0.7;
    const double top = tip.y() - size * 1.35;
    const double shoulder = tip.y() - size * 0.35;

    QPolygonF marker;
    marker << QPointF(tip.x() - halfWidth, top)
           << QPointF(tip.x() + halfWidth, top)
           << QPointF(tip.x() + halfWidth, shoulder)
           << tip
           << QPointF(tip.x() - halfWidth, shoulder);

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(borderColor, 2));
    painter.setBrush(fillColor);
    painter.drawPolygon(marker);

    QFont markerFont = painter.font();
    markerFont.setBold(true);
    markerFont.setPointSizeF(qBound(6.0, size * 0.36, 10.0));
    painter.setFont(markerFont);
    painter.setPen(Qt::black);
    QRectF textRect(tip.x() - halfWidth, top, halfWidth * 2.0, size);
    painter.drawText(textRect, Qt::AlignCenter, text);
    painter.restore();
}

// 地图加载范围的默认值，供默认/自定义/Excel加载窗口复用。
struct MapLoadRangeDefaults
{
    int startRow = 1;//建议起始行，1基
    int endRow = 1;//建议结束行，1基
    int startColumn = 1;//建议起始列，1基
    int endColumn = 1;//建议结束列，1基
    int totalRows = 1;//文件可选择的总行数
    int maxColumns = 1;//文件可选择的最大列数
};

// 去掉文本行尾的回车换行，避免按列截取时多算一列。
QByteArray trimmedLineBreaks(QByteArray line)
{
    while (!line.isEmpty() && (line.endsWith('\n') || line.endsWith('\r')))
    {
        line.chop(1);
    }
    return line;
}

// 按常见分隔符拆分一行地图/CSV数据，并清理单元格外层引号。
QStringList splitMapDataLine(const QByteArray& rawLine)
{
    QString line = QString::fromLocal8Bit(trimmedLineBreaks(rawLine));
    QChar delimiter;
    if (line.contains(','))
    {
        delimiter = ',';
    }
    else if (line.contains('\t'))
    {
        delimiter = '\t';
    }
    else if (line.contains(';'))
    {
        delimiter = ';';
    }

    QStringList parts = delimiter.isNull()
            ? line.simplified().split(' ', Qt::SkipEmptyParts)
            : line.split(delimiter, Qt::KeepEmptyParts);
    for (QString& part : parts)
    {
        part = part.trimmed();
        if (part.size() >= 2 && part.startsWith('"') && part.endsWith('"'))
        {
            part = part.mid(1, part.size() - 2);
        }
    }
    return parts;
}

// 判断文件路径是否为当前支持的Excel压缩格式。
bool isXlsxFilePath(const QString& filePath)
{
    QString suffix = QFileInfo(filePath).suffix().toLower();
    return suffix == QStringLiteral("xlsx") ||
           suffix == QStringLiteral("xlsm") ||
           suffix == QStringLiteral("xltx");
}

// Excel单元格坐标，行列均按Excel习惯从1开始保存。
struct XlsxCellRef
{
    int row = 0;//单元格行号，1基
    int column = 0;//单元格列号，1基
};

// 记录Excel工作表中某一行实际出现的单元格范围，用来推断地图主体区域。
struct XlsxRowSpan
{
    int row = 0;
    int firstColumn = INT_MAX;
    int lastColumn = 0;
    int cellCount = 0;
};

// 记录一类Excel连续行跨度出现的次数，用于找出最像地图主体的区域。
struct XlsxSpanProfile
{
    int firstColumn = 1;//该跨度的起始列
    int lastColumn = 1;//该跨度的结束列
    int cellCount = 0;//该跨度内单元格数量
    int rowCount = 0;//同类跨度连续/重复出现的行数
    int firstRow = 0;//首次出现行
    int lastRow = 0;//最后出现行
};

// 将Excel列名（A、AA等）转换为从1开始的列号。
int xlsxColumnNameToNumber(const QString& columnName)
{
    int number = 0;
    for (QChar ch : columnName)
    {
        if (!ch.isLetter())
        {
            break;
        }
        number = number * 26 + (ch.toUpper().unicode() - QLatin1Char('A').unicode() + 1);
    }
    return number;
}

// 解析Excel单元格引用，如B12拆成行号12、列号2。
XlsxCellRef parseXlsxCellRef(const QString& cellRef)
{
    QString columnName;
    QString rowNumber;
    for (QChar ch : cellRef)
    {
        if (ch.isLetter())
        {
            columnName.append(ch);
        }
        else if (ch.isDigit())
        {
            rowNumber.append(ch);
        }
    }

    XlsxCellRef result;
    result.column = xlsxColumnNameToNumber(columnName);
    result.row = rowNumber.toInt();
    return result;
}

// 读取sharedStrings.xml，Excel里 t="s" 的单元格会通过索引引用这里的文本。
QStringList readXlsxSharedStrings(const QZipReader& zipReader)
{
    QStringList sharedStrings;
    QByteArray xml = zipReader.fileData(QStringLiteral("xl/sharedStrings.xml"));
    if (xml.isEmpty())
    {
        return sharedStrings;
    }

    QXmlStreamReader reader(xml);
    bool inSharedItem = false;
    QString currentText;
    while (!reader.atEnd())
    {
        reader.readNext();
        if (reader.isStartElement() && reader.name() == QLatin1String("si"))
        {
            inSharedItem = true;
            currentText.clear();
        }
        else if (reader.isStartElement() && inSharedItem && reader.name() == QLatin1String("t"))
        {
            currentText += reader.readElementText(QXmlStreamReader::IncludeChildElements);
        }
        else if (reader.isEndElement() && reader.name() == QLatin1String("si"))
        {
            sharedStrings.append(currentText);
            inSharedItem = false;
        }
    }

    return sharedStrings;
}

// workbook关系文件里的Target可能是相对路径，这里统一成zip内路径。
QString resolveXlsxTargetPath(const QString& target)
{
    QString path = target;
    path.replace('\\', '/');
    if (path.startsWith('/'))
    {
        path.remove(0, 1);
    }
    else if (!path.startsWith(QStringLiteral("xl/")))
    {
        path = QStringLiteral("xl/") + path;
    }
    return QDir::cleanPath(path);
}

// 根据页索引找到实际worksheet xml路径。
QString resolveXlsxWorksheetPath(const QZipReader& zipReader, int pageIndex)
{
    QByteArray workbookXml = zipReader.fileData(QStringLiteral("xl/workbook.xml"));
    QByteArray relsXml = zipReader.fileData(QStringLiteral("xl/_rels/workbook.xml.rels"));
    QVector<QString> sheetRelationIds;
    QHash<QString, QString> relationTargets;

    QXmlStreamReader workbookReader(workbookXml);
    while (!workbookReader.atEnd())
    {
        workbookReader.readNext();
        if (workbookReader.isStartElement() && workbookReader.name() == QLatin1String("sheet"))
        {
            QString relationId = workbookReader.attributes()
                    .value(QStringLiteral("http://schemas.openxmlformats.org/officeDocument/2006/relationships"),
                           QStringLiteral("id")).toString();
            if (relationId.isEmpty())
            {
                relationId = workbookReader.attributes().value(QStringLiteral("r:id")).toString();
            }
            if (!relationId.isEmpty())
            {
                sheetRelationIds.append(relationId);
            }
        }
    }

    QXmlStreamReader relsReader(relsXml);
    while (!relsReader.atEnd())
    {
        relsReader.readNext();
        if (relsReader.isStartElement() && relsReader.name() == QLatin1String("Relationship"))
        {
            QString id = relsReader.attributes().value(QStringLiteral("Id")).toString();
            QString target = relsReader.attributes().value(QStringLiteral("Target")).toString();
            if (!id.isEmpty() && !target.isEmpty())
            {
                relationTargets.insert(id, resolveXlsxTargetPath(target));
            }
        }
    }

    if (pageIndex >= 0 && pageIndex < sheetRelationIds.size())
    {
        QString target = relationTargets.value(sheetRelationIds.at(pageIndex));
        if (!target.isEmpty() && !zipReader.fileData(target).isEmpty())
        {
            return target;
        }
    }

    QString fallback = QStringLiteral("xl/worksheets/sheet%1.xml").arg(pageIndex + 1);
    if (!zipReader.fileData(fallback).isEmpty())
    {
        return fallback;
    }
    return QString();
}

// 只扫描行/列跨度，不解析完整单元格内容，用于快速猜默认加载范围。
QVector<XlsxRowSpan> readXlsxRowSpans(const QByteArray& sheetXml)
{
    QVector<XlsxRowSpan> spans;
    QXmlStreamReader reader(sheetXml);
    int lastRow = 0;

    while (!reader.atEnd())
    {
        reader.readNext();
        if (!reader.isStartElement() || reader.name() != QLatin1String("row"))
        {
            continue;
        }

        XlsxRowSpan span;
        span.row = reader.attributes().value(QStringLiteral("r")).toInt();
        if (span.row <= 0)
        {
            span.row = lastRow + 1;
        }
        lastRow = span.row;

        while (!reader.atEnd())
        {
            reader.readNext();
            if (reader.isEndElement() && reader.name() == QLatin1String("row"))
            {
                break;
            }
            if (reader.isStartElement() && reader.name() == QLatin1String("c"))
            {
                XlsxCellRef cell = parseXlsxCellRef(reader.attributes().value(QStringLiteral("r")).toString());
                if (cell.column > 0)
                {
                    span.firstColumn = qMin(span.firstColumn, cell.column);
                    span.lastColumn = qMax(span.lastColumn, cell.column);
                    span.cellCount++;
                }
                reader.skipCurrentElement();
            }
        }

        if (span.cellCount > 0)
        {
            spans.append(span);
        }
    }

    return spans;
}

// Excel加载默认范围：选取重复度最高的大块单元格区域作为地图主体。
MapLoadRangeDefaults detectExcelLoadRangeDefaults(const QString& filePath, int pageIndex)
{
    MapLoadRangeDefaults defaults;
    QZipReader zipReader(filePath);
    if (!zipReader.exists())
    {
        return defaults;
    }

    QString sheetPath = resolveXlsxWorksheetPath(zipReader, qMax(0, pageIndex));
    QByteArray sheetXml = sheetPath.isEmpty() ? QByteArray() : zipReader.fileData(sheetPath);
    if (sheetXml.isEmpty())
    {
        return defaults;
    }

    QVector<XlsxRowSpan> spans = readXlsxRowSpans(sheetXml);
    if (spans.isEmpty())
    {
        return defaults;
    }

    QHash<QString, XlsxSpanProfile> profiles;
    for (const XlsxRowSpan& span : spans)
    {
        defaults.totalRows = qMax(defaults.totalRows, span.row);
        defaults.maxColumns = qMax(defaults.maxColumns, span.lastColumn);

        if (span.cellCount < 10 || span.firstColumn == INT_MAX)
        {
            continue;
        }

        QString key = QStringLiteral("%1-%2-%3")
                .arg(span.firstColumn)
                .arg(span.lastColumn)
                .arg(span.cellCount);
        XlsxSpanProfile profile = profiles.value(key);
        profile.firstColumn = span.firstColumn;
        profile.lastColumn = span.lastColumn;
        profile.cellCount = span.cellCount;
        profile.rowCount++;
        if (profile.firstRow == 0)
        {
            profile.firstRow = span.row;
        }
        profile.lastRow = span.row;
        profiles.insert(key, profile);
    }

    XlsxSpanProfile bestProfile;
    qint64 bestScore = -1;
    for (auto it = profiles.constBegin(); it != profiles.constEnd(); ++it)
    {
        const XlsxSpanProfile& profile = it.value();
        qint64 score = (qint64)profile.rowCount * profile.cellCount;
        if (score > bestScore)
        {
            bestScore = score;
            bestProfile = profile;
        }
    }

    if (bestScore <= 0)
    {
        return defaults;
    }

    defaults.startColumn = bestProfile.firstColumn;
    defaults.endColumn = bestProfile.lastColumn;
    defaults.startRow = bestProfile.firstRow;
    defaults.endRow = bestProfile.lastRow;

    bool inMapBlock = false;
    int previousRow = 0;
    for (const XlsxRowSpan& span : spans)
    {
        bool coversBestColumns = span.firstColumn <= bestProfile.firstColumn &&
                span.lastColumn >= bestProfile.lastColumn;
        if (!coversBestColumns)
        {
            if (inMapBlock && span.row > previousRow)
            {
                break;
            }
            continue;
        }

        if (!inMapBlock)
        {
            defaults.startRow = span.row;
            defaults.endRow = span.row;
            previousRow = span.row;
            inMapBlock = true;
        }
        else if (span.row <= previousRow + 1)
        {
            defaults.endRow = span.row;
            previousRow = span.row;
        }
        else
        {
            break;
        }
    }

    return defaults;
}

// 统一Excel单元格文本，空值按空白bin处理。
QString normalizeXlsxCellText(const QString& text)
{
    QString trimmed = text.trimmed();
    return trimmed.isEmpty() ? QStringLiteral(" ") : trimmed;
}

bool readWaferMapRows(const QString& filePath, QVector<QByteArray>& rows, MapLoadRangeDefaults* defaults = nullptr);

// 扫描普通文本地图，按重复最多的行宽推断默认加载范围。
MapLoadRangeDefaults detectMapLoadRangeDefaults(const QString& filePath)
{
    QVector<QByteArray> waferRows;
    MapLoadRangeDefaults waferDefaults;
    if (readWaferMapRows(filePath, waferRows, &waferDefaults))
    {
        return waferDefaults;
    }

    MapLoadRangeDefaults defaults;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return defaults;
    }

    QMap<int, int> lengthCounts;
    QMap<int, int> lengthFirstRow;
    QMap<int, int> lengthLastRow;
    int rowNumber = 0;
    int maxColumns = 0;

    while (!file.atEnd())
    {
        QByteArray line = trimmedLineBreaks(file.readLine());
        ++rowNumber;
        int length = line.size();
        maxColumns = qMax(maxColumns, length);
        if (!lengthFirstRow.contains(length))
        {
            lengthFirstRow[length] = rowNumber;
        }
        lengthLastRow[length] = rowNumber;
        lengthCounts[length]++;
    }

    defaults.totalRows = qMax(1, rowNumber);
    defaults.maxColumns = qMax(1, maxColumns);

    int bestLength = defaults.maxColumns;
    int bestCount = -1;
    for (auto it = lengthCounts.constBegin(); it != lengthCounts.constEnd(); ++it)
    {
        if (it.key() > 0 && it.value() > bestCount)
        {
            bestLength = it.key();
            bestCount = it.value();
        }
    }

    defaults.startRow = lengthFirstRow.value(bestLength, 1);
    defaults.endRow = lengthLastRow.value(bestLength, defaults.totalRows);
    defaults.startColumn = 1;
    defaults.endColumn = qMax(1, bestLength);
    return defaults;
}

// 直接解析IBIS/XML格式.map中的WAFER_MAP，不再生成中间txt文件。
bool readWaferMapRows(const QString& filePath, QVector<QByteArray>& rows, MapLoadRangeDefaults* defaults)
{
    QString suffix = QFileInfo(filePath).suffix().toLower();
    if (suffix != QStringLiteral("map") && suffix != QStringLiteral("xml"))
    {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }

    QString content = QString::fromLocal8Bit(file.readAll());
    QRegularExpression rowRegex(QStringLiteral("<ROW_COUNT>\\s*(\\d+)\\s*</ROW_COUNT>"));
    QRegularExpression columnRegex(QStringLiteral("<COLUMN_COUNT>\\s*(\\d+)\\s*</COLUMN_COUNT>"));
    QRegularExpression waferRegex(QStringLiteral("<WAFER_MAP>([\\s\\S]*?)</WAFER_MAP>"));
    QRegularExpressionMatch rowMatch = rowRegex.match(content);
    QRegularExpressionMatch columnMatch = columnRegex.match(content);
    QRegularExpressionMatch waferMatch = waferRegex.match(content);
    if (!columnMatch.hasMatch() || !waferMatch.hasMatch())
    {
        return false;
    }

    int columnCount = columnMatch.captured(1).toInt();
    if (columnCount <= 0)
    {
        return false;
    }

    QString waferText = waferMatch.captured(1);
    waferText.remove(QLatin1Char('\r'));
    waferText.remove(QLatin1Char('\n'));

    int declaredRows = rowMatch.hasMatch() ? rowMatch.captured(1).toInt() : 0;
    int calculatedRows = (waferText.size() + columnCount - 1) / columnCount;
    int rowCount = declaredRows > 0 ? qMin(declaredRows, calculatedRows) : calculatedRows;
    if (rowCount <= 0)
    {
        return false;
    }

    rows.clear();
    rows.reserve(rowCount);
    for (int row = 0; row < rowCount; ++row)
    {
        QString rowText = waferText.mid(row * columnCount, columnCount);
        if (rowText.size() < columnCount)
        {
            rowText.append(QString(columnCount - rowText.size(), QLatin1Char(' ')));
        }
        rows.append(rowText.toLocal8Bit());
    }

    if (defaults != nullptr)
    {
        defaults->startRow = 1;
        defaults->endRow = rowCount;
        defaults->startColumn = 1;
        defaults->endColumn = columnCount;
        defaults->totalRows = rowCount;
        defaults->maxColumns = columnCount;
    }
    return true;
}

}

// 生成从当前点到目标点的可行路径；目标点由蛇形规则决定，这里只负责必要绕路。
QVector<QPoint> buildAssistPathToTarget(const MapData& m_mapDat,
                                        const std::vector<std::string>& targetChars,
                                        Direction direction,
                                        int startRow,
                                        int startCol,
                                        int targetRow,
                                        int targetCol,
                                        int currentMinorDx,
                                        int currentMinorDy);

// 按蛇形扫描规则寻找下一颗目标bin，返回值为(row, col)。
std::pair<int, int> traverseSMatrix(const MapData& m_mapDat,
                                    const std::vector<std::string>& targetChars,
                                    Direction direction,
                                    int startRow,
                                    int startCol,
                                    int& currentMinorDx,
                                    int& currentMinorDy);

// 初始化界面控件、地图事件过滤器、刻度尺和功能区信号。
MapTest::MapTest(QWidget* parent)
	: QMainWindow(parent)
	, ui(new Ui::MapTest)
{
	ui->setupUi(this);

    ui->main_windows_label->setMouseTracking(true);
    ui->small_label->setMouseTracking(true);
    ui->main_windows_label->setFocusPolicy(Qt::StrongFocus);
    ui->main_windows_label->setScaledContents(false);
    setupMapTableView();
    captureSmallMapChildLayoutBase();

    BigMapHorizontalslider = new RulerSlider(Qt::Horizontal, ui->centralwidget);
    BigMapHorizontalslider->setSingleStep(1);//刻度之间的距离
    BigMapHorizontalslider->setRulerSliderRange(0,200);//设置刻度范围
    BigMapHorizontalslider->setRulerSliderValue(200);//设置当前值

    BigMapVerticalslider = new RulerSlider(Qt::Vertical, ui->centralwidget);
    BigMapVerticalslider->setSingleStep(1);//刻度之间的距离
    BigMapVerticalslider->setRulerSliderRange(0,200);//设置刻度范围
    BigMapVerticalslider->setRulerSliderValue(200);//设置当前值


    SmallMapHorizontalslider = new RulerSlider(Qt::Horizontal, ui->centralwidget);
    SmallMapHorizontalslider->setSingleStep(1);//刻度之间的距离
    SmallMapHorizontalslider->setRulerSliderRange(0,15);//设置刻度范围
    SmallMapHorizontalslider->setRulerSliderValue(15);//设置当前值

    SmallMapVerticalslider = new RulerSlider(Qt::Vertical, ui->centralwidget);
    SmallMapVerticalslider->setSingleStep(1);//刻度之间的距离
    SmallMapVerticalslider->setRulerSliderRange(0,15);//设置刻度范围
    SmallMapVerticalslider->setRulerSliderValue(15);//设置当前值

    syncMapWidgetGeometry();

    ui->small_label->installEventFilter(this);
    ui->main_windows_label->installEventFilter(this);
    ui->big_map_module->installEventFilter(this);
    ui->small_map_module->installEventFilter(this);

    m_pTimer = new QTimer(this);

    connect(m_pTimer, &QTimer::timeout, this, &MapTest::handleTimeout);

    setupMapFunctionPanel();
    updateDirectionButton();
}

// 释放界面资源；其余QObject子对象由父子关系自动销毁。
MapTest::~MapTest()
{
    delete ui;
}

// 对外接口：按公开枚举加载地图。
bool MapTest::loadMapFile(const QString& filePath, MapLoadMode loadMode)
{
    return loadMapFile(filePath, static_cast<int>(loadMode));
}

// 对外接口：按路径加载地图。
bool MapTest::loadMapFile(const QString& filePath, int loadMode)
{
    return loadMapFileFromPath(filePath, loadMode);
}

// 对外接口：按扫码内容查找地图文件。
QString MapTest::searchMapFileByScanText(const QString& scanText, const QStringList& searchRoots) const
{
    return findMapFileByScanText(scanText, searchRoots);
}

// 对外接口：按扫码内容查找并加载地图。
bool MapTest::loadMapByScanText(const QString& scanText,
                                const QStringList& searchRoots,
                                MapLoadMode loadMode,
                                QString* matchedFilePath)
{
    const QString mapFilePath = searchMapFileByScanText(scanText, searchRoots);
    if (matchedFilePath)
    {
        *matchedFilePath = mapFilePath;
    }
    if (mapFilePath.isEmpty())
    {
        return false;
    }

    return loadMapFile(mapFilePath, loadMode);
}

// 对外接口：地图是否已加载。
bool MapTest::hasMapLoaded() const
{
    return !m_mapData.isEmpty();
}

// 对外接口：当前地图文件路径。
QString MapTest::currentMapFilePath() const
{
    return filTexyename;
}

// 对外接口：原始地图列数。
int MapTest::mapColumnCount() const
{
    return map_columns;
}

// 对外接口：原始地图行数。
int MapTest::mapRowCount() const
{
    return map_rows;
}

// 对外接口：当前显示方向下的列数。
int MapTest::displayColumnCount() const
{
    return displayColumns();
}

// 对外接口：当前显示方向下的行数。
int MapTest::displayRowCount() const
{
    return displayRows();
}

// 对外接口：返回原始地图容器，只读；引用在重新加载/编辑地图后可能变化。
const MapData& MapTest::sourceMapData() const
{
    return m_mapData;
}

// 对外接口：返回当前显示方向下的地图容器，只读；0度时就是原始地图。
const MapData& MapTest::displayMapData() const
{
    return currentMapData();
}

// 对外接口：获取原始地图指定坐标内容。
QString MapTest::sourceCellValue(int column, int row) const
{
    if (column < 0 || row < 0 || column >= map_columns || row >= map_rows)
    {
        return QString();
    }

    return QString::fromStdString(mapCellValue(m_mapData, row, column));
}

// 对外接口：获取当前显示地图指定坐标内容。
QString MapTest::displayCellValue(int column, int row) const
{
    if (column < 0 || row < 0 || column >= displayColumns() || row >= displayRows())
    {
        return QString();
    }

    return QString::fromStdString(mapCellValue(currentMapData(), row, column));
}

// 对外接口：当前行进方向。
Direction MapTest::mapDirection() const
{
    return m_selectedDirection;
}

// 对外接口：设置行进方向，运行中不允许切换。
bool MapTest::setMapDirection(Direction direction)
{
    const int directionId = static_cast<int>(direction);
    if (directionId < LeftToRightTopToBottom || directionId > BottomToTopRightToLeft)
    {
        return false;
    }
    if (m_pTimer->isActive())
    {
        return false;
    }

    m_selectedDirection = direction;
    MapDirectionVectors vectors = directionVectors(m_selectedDirection);
    m_currentMinorDx = vectors.minor.dx;
    m_currentMinorDy = vectors.minor.dy;
    m_pendingPathDisplayCells.clear();
    m_assistPathSourceCells.clear();
    m_hasFoundStartPoint = false;
    clearFindStartRoute();
    invalidateMapColorImage();
    updateDirectionButton();
    refreshMapViews();
    return true;
}

// 对外接口：获取当前显示坐标。
QPoint MapTest::currentDisplayCell() const
{
    return QPoint(RecordCurrent_x, RecordCurrent_y);
}

// 对外接口：选中当前显示方向下的指定格。
bool MapTest::selectDisplayCell(int column, int row, bool centerView, bool refresh)
{
    if (m_mapData.isEmpty() ||
        column < 0 || row < 0 ||
        column >= displayColumns() || row >= displayRows())
    {
        return false;
    }

    setSelectedCell(column, row, centerView, refresh);
    return true;
}

// 对外接口：设置起点。
bool MapTest::setStartPoint(int column, int row)
{
    if (!selectDisplayCell(column, row, true, false))
    {
        return false;
    }

    QPoint sourceCell = displayToSourceCell(RecordCurrent_x, RecordCurrent_y);
    sourceCell.setX(clampToRange(sourceCell.x(), 0, map_columns - 1));
    sourceCell.setY(clampToRange(sourceCell.y(), 0, map_rows - 1));
    m_startPointSource = sourceCell;
    m_hasStartPoint = true;
    m_hasFoundStartPoint = false;
    clearFindStartRoute();
    refreshMapViews();
    return true;
}

// 对外接口：设置唯一参考点。
bool MapTest::setReferencePoint(int column, int row)
{
    if (!selectDisplayCell(column, row, true, false))
    {
        return false;
    }

    QPoint sourceCell = displayToSourceCell(RecordCurrent_x, RecordCurrent_y);
    sourceCell.setX(clampToRange(sourceCell.x(), 0, map_columns - 1));
    sourceCell.setY(clampToRange(sourceCell.y(), 0, map_rows - 1));
    m_referencePointSource = sourceCell;
    m_hasReferencePoint = true;
    m_hasFoundStartPoint = false;
    clearFindStartRoute();
    refreshMapViews();
    return true;
}

// 对外接口：追加验证点。
bool MapTest::addVerifyPoint(int column, int row)
{
    if (!selectDisplayCell(column, row, true, false))
    {
        return false;
    }

    QPoint sourceCell = displayToSourceCell(RecordCurrent_x, RecordCurrent_y);
    sourceCell.setX(clampToRange(sourceCell.x(), 0, map_columns - 1));
    sourceCell.setY(clampToRange(sourceCell.y(), 0, map_rows - 1));
    if (!m_verifyPointsSource.contains(sourceCell))
    {
        m_verifyPointsSource.append(sourceCell);
    }
    m_hasFoundStartPoint = false;
    clearFindStartRoute();
    refreshMapViews();
    return true;
}

// 对外接口：清空验证点。
void MapTest::clearVerifyPointList()
{
    m_verifyPointsSource.clear();
    m_hasFoundStartPoint = false;
    clearFindStartRoute();
    refreshMapViews();
}

// 对外接口：停止寻起点或路径规划。
void MapTest::stopFindStartPoint(const QString& reason)
{
    m_pTimer->stop();
    m_isFindingStartPoint = false;
    m_hasFoundStartPoint = false;
    clearFindStartRoute();
    setMapFunctionPanelRunning(false);
    refreshMapViews();

    if (!reason.isEmpty())
    {
        ui->label->setText(reason);
    }
}

// 对外接口：停止寻起点或路径规划。
void MapTest::stopMapPathPlanning()
{
    if (m_isFindingStartPoint || !m_pendingFindStartDisplayCells.isEmpty())
    {
        stopFindStartPoint();
        return;
    }

    m_pTimer->stop();
    m_pendingPathDisplayCells.clear();
    setMapFunctionPanelRunning(false);
    refreshMapViews();
}

// 对外接口：返回寻起点下一步显示坐标。
bool MapTest::nextFindStartDisplayCell(QPoint* displayCell)
{
    if (displayCell == nullptr || !prepareFindStartRoute() || m_pendingFindStartDisplayCells.isEmpty())
    {
        return false;
    }

    *displayCell = m_pendingFindStartDisplayCells.first();
    return true;
}

// 对外接口：返回寻起点下一步原始地图坐标。
bool MapTest::nextFindStartSourceCell(QPoint* sourceCell)
{
    if (sourceCell == nullptr)
    {
        return false;
    }

    QPoint displayCell;
    if (!nextFindStartDisplayCell(&displayCell))
    {
        return false;
    }

    *sourceCell = displayToSourceCell(displayCell.x(), displayCell.y());
    return true;
}

// 对外接口：返回寻起点当前剩余显示坐标路径。
QVector<QPoint> MapTest::pendingFindStartPathDisplayCells() const
{
    return m_pendingFindStartDisplayCells;
}

// 对外接口：返回寻起点当前剩余原始地图坐标路径。
QVector<QPoint> MapTest::pendingFindStartPathSourceCells() const
{
    QVector<QPoint> sourcePath;
    sourcePath.reserve(m_pendingFindStartDisplayCells.size());
    for (const QPoint& displayCell : m_pendingFindStartDisplayCells)
    {
        sourcePath.append(displayToSourceCell(displayCell.x(), displayCell.y()));
    }
    return sourcePath;
}

// 对外接口：寻起点前进一步。外部硬件每实际走到一颗后调用一次，界面同步标记一颗。
bool MapTest::moveToNextFindStartCell(QPoint* movedDisplayCell, bool* reachedStart)
{
    if (reachedStart)
    {
        *reachedStart = false;
    }
    if (m_mapData.isEmpty())
    {
        return false;
    }
    if (m_pendingFindStartDisplayCells.isEmpty())
    {
        if (m_hasFoundStartPoint)
        {
            if (reachedStart)
            {
                *reachedStart = true;
            }
            return false;
        }
        if (!prepareFindStartRoute())
        {
            return false;
        }
    }
    if (m_pendingFindStartDisplayCells.isEmpty())
    {
        if (reachedStart)
        {
            *reachedStart = m_hasFoundStartPoint;
        }
        return false;
    }

    QPoint nextDisplayCell = m_pendingFindStartDisplayCells.takeFirst();
    markFindStartPathCell(nextDisplayCell.x(), nextDisplayCell.y());
    setSelectedCell(nextDisplayCell.x(), nextDisplayCell.y(), true, false);

    if (movedDisplayCell)
    {
        *movedDisplayCell = nextDisplayCell;
    }

    const bool routeFinished = m_pendingFindStartDisplayCells.isEmpty();
    if (reachedStart)
    {
        *reachedStart = routeFinished;
    }
    if (routeFinished)
    {
        m_isFindingStartPoint = false;
        m_hasFoundStartPoint = true;
        refreshMapViews();
        ui->label->setText(QStringLiteral("已寻到起点"));
        setMapFunctionPanelRunning(false);
        return true;
    }

    refreshMapViews();
    return true;
}

// 对外接口：将当前点标记为已吸取，并同步表格数量和大小图颜色。
bool MapTest::markCurrentCellPicked()
{
    bool picked = processCurrentCellIfChecked();
    refreshMapViews();
    return picked;
}

// 对外接口：选中指定点后标记为已吸取。
bool MapTest::markDisplayCellPicked(int column, int row)
{
    if (!selectDisplayCell(column, row, true, false))
    {
        return false;
    }

    return markCurrentCellPicked();
}

// 对外接口：计算当前点到下一颗吸取点的路径，路径最后一个点就是下一颗吸取点。
bool MapTest::prepareNextPickupRoute()
{
    if (m_mapData.isEmpty())
    {
        return false;
    }
    if (!m_hasFoundStartPoint)
    {
        return false;
    }
    if (!m_pendingPathDisplayCells.isEmpty())
    {
        return true;
    }

    std::unordered_map<std::string, int> checkedRows = checkedTargetRows();
    if (checkedRows.empty())
    {
        return false;
    }

    std::vector<std::string> targetChars;
    targetChars.reserve(checkedRows.size());
    for (const auto& checkedRow : checkedRows)
    {
        targetChars.push_back(checkedRow.first);
    }

    int nextMinorDx = m_currentMinorDx;
    int nextMinorDy = m_currentMinorDy;
    const MapData& searchMapData = currentMapData();
    std::pair<int, int> nextTarget = traverseSMatrix(searchMapData,
                                                     targetChars,
                                                     m_selectedDirection,
                                                     RecordCurrent_y,
                                                     RecordCurrent_x,
                                                     nextMinorDx,
                                                     nextMinorDy);
    if (nextTarget.first < 0 || nextTarget.second < 0)
    {
        return false;
    }

    QVector<QPoint> nextPath = buildAssistPathToTarget(searchMapData,
                                                       targetChars,
                                                       m_selectedDirection,
                                                       RecordCurrent_y,
                                                       RecordCurrent_x,
                                                       nextTarget.first,
                                                       nextTarget.second,
                                                       m_currentMinorDx,
                                                       m_currentMinorDy);
    if (nextPath.isEmpty())
    {
        return false;
    }

    m_pendingPathDisplayCells = nextPath;
    m_currentMinorDx = nextMinorDx;
    m_currentMinorDy = nextMinorDy;
    return true;
}

// 对外接口：返回下一颗吸取点显示坐标。
bool MapTest::nextPickupDisplayCell(QPoint* displayCell)
{
    if (displayCell == nullptr || !prepareNextPickupRoute())
    {
        return false;
    }

    *displayCell = m_pendingPathDisplayCells.last();
    return true;
}

// 对外接口：返回下一颗吸取点原始地图坐标。
bool MapTest::nextPickupSourceCell(QPoint* sourceCell)
{
    if (sourceCell == nullptr)
    {
        return false;
    }

    QPoint displayCell;
    if (!nextPickupDisplayCell(&displayCell))
    {
        return false;
    }

    *sourceCell = displayToSourceCell(displayCell.x(), displayCell.y());
    return true;
}

// 对外接口：返回到下一颗吸取点的完整显示坐标路径。
QVector<QPoint> MapTest::nextMovePathDisplayCells()
{
    if (!prepareNextPickupRoute())
    {
        return QVector<QPoint>();
    }

    return m_pendingPathDisplayCells;
}

// 对外接口：返回到下一颗吸取点的完整原始地图坐标路径。
QVector<QPoint> MapTest::nextMovePathSourceCells()
{
    if (!prepareNextPickupRoute())
    {
        return QVector<QPoint>();
    }

    return pendingMovePathSourceCells();
}

// 对外接口：返回绕路/经过路径显示坐标，不包含最后吸取点。
QVector<QPoint> MapTest::nextAssistPathDisplayCells()
{
    if (!prepareNextPickupRoute() || m_pendingPathDisplayCells.size() <= 1)
    {
        return QVector<QPoint>();
    }

    QVector<QPoint> assistPath = m_pendingPathDisplayCells;
    assistPath.removeLast();
    return assistPath;
}

// 对外接口：返回绕路/经过路径原始地图坐标，不包含最后吸取点。
QVector<QPoint> MapTest::nextAssistPathSourceCells()
{
    QVector<QPoint> assistDisplayPath = nextAssistPathDisplayCells();
    QVector<QPoint> assistSourcePath;
    assistSourcePath.reserve(assistDisplayPath.size());
    for (const QPoint& displayCell : assistDisplayPath)
    {
        assistSourcePath.append(displayToSourceCell(displayCell.x(), displayCell.y()));
    }
    return assistSourcePath;
}

// 对外接口：沿待走路径前进一步。isPickupCell=true表示该步到达下一颗吸取点。
bool MapTest::moveToNextPlannedCell(QPoint* movedDisplayCell, bool* isPickupCell)
{
    if (!prepareNextPickupRoute())
    {
        return false;
    }

    QPoint nextDisplayCell = m_pendingPathDisplayCells.takeFirst();
    bool finalTargetStep = m_pendingPathDisplayCells.isEmpty();
    if (!finalTargetStep)
    {
        markAssistPathCell(nextDisplayCell.x(), nextDisplayCell.y());
    }

    if (movedDisplayCell)
    {
        *movedDisplayCell = nextDisplayCell;
    }
    if (isPickupCell)
    {
        *isPickupCell = finalTargetStep;
    }

    setSelectedCell(nextDisplayCell.x(), nextDisplayCell.y(), true, false);
    refreshMapViews();
    return true;
}

// 对外接口：返回当前尚未行走的显示坐标路径。
QVector<QPoint> MapTest::pendingMovePathDisplayCells() const
{
    return m_pendingPathDisplayCells;
}

// 对外接口：返回当前尚未行走的原始地图坐标路径。
QVector<QPoint> MapTest::pendingMovePathSourceCells() const
{
    QVector<QPoint> sourcePath;
    sourcePath.reserve(m_pendingPathDisplayCells.size());
    for (const QPoint& displayCell : m_pendingPathDisplayCells)
    {
        sourcePath.append(displayToSourceCell(displayCell.x(), displayCell.y()));
    }
    return sourcePath;
}

bool MapTest::hasFoundStartPoint() const
{
    return m_hasFoundStartPoint;
}

bool MapTest::isFindingStartPoint() const
{
    return m_isFindingStartPoint;
}

bool MapTest::isPathPlanningRunning() const
{
    return m_pTimer->isActive() && !m_isFindingStartPoint;
}

bool MapTest::hasStartPoint() const
{
    return m_hasStartPoint;
}

bool MapTest::hasReferencePoint() const
{
    return m_hasReferencePoint;
}

QPoint MapTest::startPoint() const
{
    return m_startPointSource;
}

QPoint MapTest::referencePoint() const
{
    return m_referencePointSource;
}

QVector<QPoint> MapTest::verifyPoints() const
{
    return m_verifyPointsSource;
}

namespace
{

// 获取或创建表格单元格里的居中复选框。
QCheckBox* ensureTableCheckBox(QTableWidget* table, int row, int column)
{
    QWidget* cellWidget = table->cellWidget(row, column);
    if (cellWidget == nullptr)
    {
        cellWidget = new QWidget(table);
        QHBoxLayout* layout = new QHBoxLayout(cellWidget);
        QCheckBox* checkBox = new QCheckBox(cellWidget);
        checkBox->setFont(table->font());
        checkBox->setFocusPolicy(Qt::NoFocus);

        if (QStyle* fusionStyle = QStyleFactory::create("fusion"))
        {
            fusionStyle->setParent(checkBox);
            checkBox->setStyle(fusionStyle);
        }

        checkBox->setStyleSheet(QStringLiteral("QCheckBox { margin: 1px; border: 0; }"
                                                "QCheckBox::indicator { width: 16px; height: 16px; }"));
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(checkBox, 0, Qt::AlignCenter);
        table->setCellWidget(row, column, cellWidget);
    }

    return cellWidget->findChild<QCheckBox*>();
}

// 设置表格指定单元格的加工选择状态。
void setTableCheckBox(QTableWidget* table, int row, int column, bool checked)
{
    QCheckBox* checkBox = ensureTableCheckBox(table, row, column);
    if (checkBox != nullptr)
    {
        checkBox->setChecked(checked);
    }
}

}

//使用QImageReader的预设缩放来加载本地图片
QPixmap MapTest::LoadImageFunction(const QString &imagePath,const QSize &targetsize)
{

    QImageReader imageReader;
    //设置图片文件路径，注意：设置文件路径并不会产生文件I0动作。
    imageReader.setFileName(imagePath);
    imageReader.setAutoTransform(true);
    //图片原大小
    auto imageSize=imageReader.size();
    //如果想要按原图比例缩放，就需要多做一次尺寸变换。
    auto targetScaledsize =imageSize.scaled(targetsize,Qt::KeepAspectRatio);

    //设置预设缩放尺寸
    imageReader.setScaledSize(targetScaledsize);

    QPixmap pixmap=QPixmap::fromImageReader(&imageReader);

    return pixmap;
}

//是否包含空格
bool hasSpace(const std::string& str)
{
    return str.find(' ') != std::string::npos;
}

// 判断一行中是否有空格符按照规律大量出现超过总字符数的1/3
bool hasSpacePattern(const std::string& line)
{
    size_t totalCharacters = line.length();
    size_t spaceCount = 0;

    // 统计空格字符个数
    for (char c : line)
    {
        if (c == ' ')
        {
            spaceCount++;
        }
    }

    // 判断空格字符个数是否超过总字符数的1/3
    return spaceCount > (totalCharacters / 3);
}

// 找到符合左右两侧空格相等且不等于原始字符串的子串并存储在容器中
std::vector<std::string> findAndStore(const std::string& input) {
    std::vector<std::string> resultContainer;
    size_t startPos = 0;

    while (startPos < input.length()) {
        // 寻找第一个非空字符的位置
        size_t firstNonSpace = input.find_first_not_of(' ', startPos);

        if (firstNonSpace != std::string::npos) {
            // 寻找从第一个非空字符开始到下一个空格之前的子串
            size_t nextSpace = input.find(' ', firstNonSpace);

            // 如果没有找到下一个空格，将其设置为字符串末尾的位置
            if (nextSpace == std::string::npos) {
                nextSpace = input.length();
            }

            // 如果子串中存在空格，才继续处理
            if (nextSpace > firstNonSpace) {
                std::string result = input.substr(firstNonSpace, nextSpace - firstNonSpace);

                // 计算左右两侧的空格数量
                size_t leftSpaces = 0;
                size_t rightSpaces = 0;

                // 计算左侧的空格数量
                for (size_t i = firstNonSpace - 1; i != SIZE_MAX && input[i] == ' '; --i) {
                    ++leftSpaces;
                }

                // 计算右侧的空格数量
                for (size_t i = nextSpace; i < input.length() && input[i] == ' '; ++i) {
                    ++rightSpaces;
                }

                // 检查左右两侧的空格数量是否相等且子串不等于原始字符串
                if (leftSpaces == rightSpaces && result != input) {
                    resultContainer.push_back(result);
                }
            }

            // 更新搜索的起始位置
            startPos = nextSpace + 1;
        } else {
            // 如果没有找到非空字符，则结束循环
            break;
        }
    }

    return resultContainer;
}

// 删除没有与中间空格规律相同的行
void removeLinesWithoutSpacePattern(QVector<QByteArray>& mapData)
{
    int middleIndex = mapData.size() / 2; // 中间行索引

        if (middleIndex < 0 || middleIndex >= mapData.size()) {
            return; // 无效的中间行索引
        }

        QByteArray middleLine = mapData[middleIndex];

        QVector<int> patternLengths;
        int currentPatternLength = 0;

        for (int i = 0; i < middleLine.size(); ++i) {
            if (middleLine.at(i) == ' ') {
                ++currentPatternLength;
            } else {
                if (currentPatternLength > 0) {
                    patternLengths.append(currentPatternLength);
                    currentPatternLength = 0;
                }
            }
        }

        // 处理最后一个连续空格序列
        if (currentPatternLength > 0) {
            patternLengths.append(currentPatternLength);
        }

        // 统计每个长度出现的次数
        QMap<int, int> patternCounts;
        for (int length : patternLengths) {
            patternCounts[length]++;
        }

        // 找到出现次数最多的长度
        int maxCount = 0;
        int maxCountLength = 0;
        for (auto it = patternCounts.begin(); it != patternCounts.end(); ++it) {
            if (it.value() > maxCount) {
                maxCount = it.value();
                maxCountLength = it.key();
            }
        }

        auto it = mapData.begin();
        while (it != mapData.end()) {
            const QByteArray& line = *it;

            int consecutiveSpaces = 0;
            bool inPattern = false;

            for (char ch : line)
            {
                if (ch == ' ')
                {
                    if (!inPattern)
                    {
                        inPattern = true;
                        ++consecutiveSpaces;
                    } else
                    {
                        ++consecutiveSpaces;
                    }
                }
                else
                {
                    inPattern = false;
                    break;
                }


            }

            if (consecutiveSpaces >= maxCountLength)
            {
                it = mapData.erase(it);
            } else
            {
                ++it;
            }
            consecutiveSpaces = 0;
        }
}

//第几列都是空格字符
int findFirstAllSpacesColumn(const QVector<QByteArray>& mapData)
{
    if (mapData.isEmpty()) {
        return -1; // 数据为空，没有列可判断
    }

    int column = -1; // 初始列索引为 -1，表示未找到满足条件的列

    const QByteArray& firstLine = mapData.first();

    // 遍历第一行的每个字符
    for (int i = 0; i < firstLine.size(); i++) {
        bool allSpaces = true; // 默认当前列都是空格

        // 检查每一行的对应列字符是否都是空格
        for (const QByteArray& line : mapData) {
            if (i >= line.size() || line[i] != ' ') {
                allSpaces = false; // 发现非空格字符或者列索引超出行长度
                break;
            }
        }

        if (allSpaces) {
            column = i; // 找到满足条件的列索引
            break;
        }
    }

    return column;
}

//从第几列数据，并将该列全是空格的数据删除
void removeColumnsFromMapData(QVector<QByteArray>& mapData, int numColumns)
{
    if (numColumns <= 0 || mapData.isEmpty()) {
            return; // 无效的列数或者数据为空，不进行删除操作
        }

        for (QByteArray& line : mapData) {
            if (numColumns >= line.size()) {
                line.clear(); // 如果要删除的列数大于等于当前行长度，则清空整行
            } else {
                line = line.mid(numColumns); // 删除前 numColumns 列
            }
        }

        QVector<QByteArray> updatedMapData;

        for (const QByteArray& line : mapData) {
            QByteArray updatedLine;
            for (int i = 0; i < line.size(); i++) {
                if (line[i] != ' ') {
                    updatedLine.append(line[i]);
                }
            }
            updatedMapData.append(updatedLine);
        }

        mapData = updatedMapData;
}

//最后一列从第几行开始与最后一个字符相同
int getLastColumnStartIndex(const QVector<QByteArray>& data)
{
    if (data.isEmpty() || data[0].isEmpty()) {
           return -1; // 数据为空，返回无效索引
       }

       int lastColumnIndex = data[0].size() - 2; // 最后一列的索引
       char lastChar = data.last()[lastColumnIndex]; // 最后一列的最后一个字符

       for (int i = 0; i < data.size(); ++i) {
           const QByteArray& line = data[i];
           if (line.size() > lastColumnIndex && line[lastColumnIndex] == lastChar) {
               return i; // 找到最后一个字符的行索引
           }
       }

       return -1; // 最后一个字符未找到，返回无效索引
}

//移除前多少行数据
void removeRows(QVector<QByteArray>& data, int numRows)
{
    data.erase(data.begin(), data.begin() + numRows); // 移除前 numRows 行数据
}

//移除前多少列数据
void removeColumns(QVector<QByteArray>& data, int numColumns)
{
    for (QByteArray& line : data) {
        if (numColumns >= line.size()) {
            line.clear(); // 如果要移除的列数大于等于当前行长度，则清空整行
        } else {
            line.remove(0, numColumns); // 移除前 numColumns 列数据
        }
    }
}

//移除空格列
void removeWhitespaceColumns(QVector<QByteArray>& data)
{
    if (data.isEmpty() || data[0].isEmpty()) {
        return; // 数据为空，无需操作
    }

    for (int columnIndex = data[0].size() - 1; columnIndex >= 0; --columnIndex) {
        bool isWhitespaceColumn = true; // 标记当前列是否为空格列

        for (const QByteArray& line : data) {
            if (line.size() <= columnIndex || line[columnIndex] != ' ') {
                isWhitespaceColumn = false;
                break;
            }
        }

        if (isWhitespaceColumn) {
            for (QByteArray& line : data) {
                line.remove(columnIndex, 1); // 移除当前空格列
            }
        }
    }
}

//移除空格行
void removeWhitespaceRows(QVector<QByteArray>& data)
{
    QVector<int> rowsToRemove; // 记录需要移除的行索引

    for (int rowIndex = data.size() - 1; rowIndex >= 0; --rowIndex) {
        const QByteArray& line = data[rowIndex];

        bool isWhitespaceRow = true; // 标记当前行是否为空格行

        for (char ch : line) {
            if (ch != ' ') {
                isWhitespaceRow = false;
                break;
            }
        }

        if (isWhitespaceRow) {
            rowsToRemove.append(rowIndex); // 添加需要移除的行索引
        }
    }

    // 根据行索引移除空格行
    for (int i = rowsToRemove.size() - 1; i >= 0; --i) {
        int rowIndex = rowsToRemove[i];
        data.remove(rowIndex);
    }
}

//移除空格行列
void removeWhitespaceRowsAndColumns(QVector<QByteArray>& data)
{
    if (data.isEmpty() || data[0].isEmpty()) {
            return; // 数据为空，无需操作
        }

        int numRows = data.size();
        int numColumns = data[0].size();
        QVector<int> rowsToRemove;
        QVector<int> columnsToRemove;

        // 检查空白列
        for (int columnIndex = numColumns - 1; columnIndex >= 0; --columnIndex) {
            bool isWhitespaceColumn = true;

            for (int rowIndex = 0; rowIndex < numRows; ++rowIndex) {
                if (data[rowIndex].size() <= columnIndex || data[rowIndex][columnIndex] != ' ') {
                    isWhitespaceColumn = false;
                    break;
                }
            }

            if (isWhitespaceColumn) {
                columnsToRemove.append(columnIndex);
            }
        }

        // 移除空白列
        for (int columnIndex : columnsToRemove) {
            for (QByteArray& line : data) {
                if (line.size() > columnIndex) {
                    line.remove(columnIndex, 1);
                }
            }
        }

        // 检查空白行
        for (int rowIndex = numRows - 1; rowIndex >= 0; --rowIndex) {
            QByteArray trimmedLine = data[rowIndex].trimmed();

            if (trimmedLine.isEmpty()) {
                rowsToRemove.append(rowIndex);
            }
        }

        // 移除空白行
        for (int rowIndex : rowsToRemove) {
            data.remove(rowIndex);
        }
}

//读取Txt按钮
void MapTest::on_read_pushButton_clicked()
{
    int loadMode = ui->map_load_mode_comboBox->currentIndex();

    QString selectedFileName = QFileDialog::getOpenFileName(this, "打开晶元图", "./",tr("*"));
    if (selectedFileName.isEmpty())
    {
        updateSelectionUi();
        return;
    }

    loadMapFile(selectedFileName, loadMode);
}

// 地图加载统一入口：按模式配置参数、清空旧状态并触发读取。
bool MapTest::loadMapFileFromPath(const QString& filePath, int loadMode)
{
    if (filePath.isEmpty())
    {
        return false;
    }

    if (isXlsxFilePath(filePath) && loadMode != ExcelLoadMode)
    {
        loadMode = ExcelLoadMode;
    }

    QString convertedFileName = filePath;

    m_activeLoadMode = loadMode;
    if (loadMode == CsvLoadMode)
    {
        if (!configureCsvLoadOptions(convertedFileName))
        {
            return false;
        }
        m_hasLoadRange = false;
    }
    else if (loadMode == CustomLoadMode)
    {
        if (!configureMapLoadRange(convertedFileName))
        {
            return false;
        }
    }
    else if (loadMode == ExcelLoadMode)
    {
        if (!configureExcelLoadRange(convertedFileName))
        {
            return false;
        }
    }
    else
    {
        m_hasLoadRange = false;
    }

    filTexyename = convertedFileName;

    m_mapData.clear();
    rotate_m_mapData.clear();
    m_hasStartPoint = false;
    m_hasReferencePoint = false;
    m_hasFoundStartPoint = false;
    m_isFindingStartPoint = false;
    m_verifyPointsSource.clear();
    m_hasEditRectStart = false;
    m_hasEditRectEnd = false;
    m_pendingPathDisplayCells.clear();
    m_processedSourceCells.clear();
    m_processedOriginalValues.clear();
    m_assistPathSourceCells.clear();
    clearFindStartRoute();

    readMapTextFile();

    ui->label->setText(" 读取完成！! !");
    return true;
}

// 根据扫码内容在指定目录中查找匹配的地图文件。
QString MapTest::findMapFileByScanText(const QString& scanText, const QStringList& searchRoots) const
{
    QString text = scanText.trimmed();
    text.remove('"');
    text.remove('\'');
    if (text.isEmpty())
    {
        return QString();
    }

    QFileInfo directFileInfo(text);
    if (directFileInfo.exists() && directFileInfo.isFile())
    {
        return directFileInfo.absoluteFilePath();
    }

    QString fileNameText = QFileInfo(text).fileName();
    if (fileNameText.isEmpty())
    {
        fileNameText = text;
    }

    QStringList candidates;
    candidates << fileNameText;
    if (QFileInfo(fileNameText).suffix().isEmpty())
    {
        candidates << fileNameText + QStringLiteral(".txt")
                   << fileNameText + QStringLiteral(".TXT")
                   << fileNameText + QStringLiteral(".map")
                   << fileNameText + QStringLiteral(".MAP")
                   << fileNameText + QStringLiteral(".csv")
                   << fileNameText + QStringLiteral(".CSV")
                   << fileNameText + QStringLiteral(".xlsx")
                   << fileNameText + QStringLiteral(".XLSX")
                   << fileNameText + QStringLiteral(".xlsm")
                   << fileNameText + QStringLiteral(".XLSM");
    }

    QStringList tokens;
    QRegularExpression tokenRegex(QStringLiteral("[A-Za-z0-9_#.-]{3,}"));
    QRegularExpressionMatchIterator matchIterator = tokenRegex.globalMatch(text);
    while (matchIterator.hasNext())
    {
        QString token = matchIterator.next().captured(0);
        if (!tokens.contains(token, Qt::CaseInsensitive))
        {
            tokens << token;
        }
    }
    std::sort(tokens.begin(), tokens.end(), [](const QString& left, const QString& right) {
        return left.length() > right.length();
    });

    QStringList roots = searchRoots;
    roots.removeDuplicates();

    QString containsMatch;
    QString tokenMatch;
    QSet<QString> visitedFiles;
    for (const QString& root : roots)
    {
        QDir rootDir(root);
        if (!rootDir.exists())
        {
            continue;
        }

        QDirIterator iterator(rootDir.absolutePath(),
                              QDir::Files | QDir::NoSymLinks,
                              QDirIterator::Subdirectories);
        while (iterator.hasNext())
        {
            QFileInfo fileInfo(iterator.next());
            QString filePath = QDir::cleanPath(fileInfo.absoluteFilePath());
            QString normalizedPath = filePath.toLower();
            if (visitedFiles.contains(normalizedPath) ||
                normalizedPath.contains(QStringLiteral("/.git/")) ||
                normalizedPath.contains(QStringLiteral("/build-")))
            {
                continue;
            }
            visitedFiles.insert(normalizedPath);

            QString fileName = fileInfo.fileName();
            QString baseName = fileInfo.completeBaseName();
            for (const QString& candidate : candidates)
            {
                if (fileName.compare(candidate, Qt::CaseInsensitive) == 0 ||
                    baseName.compare(candidate, Qt::CaseInsensitive) == 0)
                {
                    return filePath;
                }
            }

            if (containsMatch.isEmpty())
            {
                for (const QString& candidate : candidates)
                {
                    if (candidate.length() >= 3 &&
                        fileName.contains(candidate, Qt::CaseInsensitive))
                    {
                        containsMatch = filePath;
                        break;
                    }
                }
            }

            if (tokenMatch.isEmpty())
            {
                for (const QString& token : tokens)
                {
                    if (fileName.contains(token, Qt::CaseInsensitive) ||
                        baseName.contains(token, Qt::CaseInsensitive))
                    {
                        tokenMatch = filePath;
                        break;
                    }
                }
            }
        }
    }

    return !containsMatch.isEmpty() ? containsMatch : tokenMatch;
}

// 弹出普通文本地图范围设置窗口，记录要读取的行列和bin位数。
bool MapTest::configureMapLoadRange(const QString& filePath)
{
    MapLoadRangeDefaults defaults = detectMapLoadRangeDefaults(filePath);

    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("地图范围设置"));
    dialog.setModal(true);
    dialog.setMinimumWidth(360);

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);
    QLabel* fileLabel = new QLabel(QFileInfo(filePath).fileName(), &dialog);
    fileLabel->setWordWrap(true);
    mainLayout->addWidget(fileLabel);

    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->setHorizontalSpacing(12);
    gridLayout->setVerticalSpacing(10);

    QSpinBox* startRowSpinBox = new QSpinBox(&dialog);
    QSpinBox* startColumnSpinBox = new QSpinBox(&dialog);
    QSpinBox* endRowSpinBox = new QSpinBox(&dialog);
    QSpinBox* endColumnSpinBox = new QSpinBox(&dialog);
    QComboBox* binCharCountComboBox = new QComboBox(&dialog);

    startRowSpinBox->setRange(0, qMax(0, defaults.totalRows - 1));
    endRowSpinBox->setRange(0, qMax(0, defaults.totalRows - 1));
    startColumnSpinBox->setRange(0, qMax(0, defaults.maxColumns - 1));
    endColumnSpinBox->setRange(0, qMax(0, defaults.maxColumns - 1));

    startRowSpinBox->setValue(qMax(0, defaults.startRow - 1));
    endRowSpinBox->setValue(qMax(0, defaults.endRow - 1));
    startColumnSpinBox->setValue(qMax(0, defaults.startColumn - 1));
    endColumnSpinBox->setValue(qMax(0, defaults.endColumn - 1));
    binCharCountComboBox->addItems(QStringList()
                                   << QStringLiteral("1位")
                                   << QStringLiteral("2位")
                                   << QStringLiteral("3位")
                                   << QStringLiteral("4位"));
    binCharCountComboBox->setCurrentIndex(clampToRange(m_loadBinCharCount, 1, MaxLoadBinCharCount) - 1);

    gridLayout->addWidget(new QLabel(QStringLiteral("起始行"), &dialog), 0, 0);
    gridLayout->addWidget(startRowSpinBox, 0, 1);
    gridLayout->addWidget(new QLabel(QStringLiteral("起始列"), &dialog), 1, 0);
    gridLayout->addWidget(startColumnSpinBox, 1, 1);
    gridLayout->addWidget(new QLabel(QStringLiteral("结束行"), &dialog), 2, 0);
    gridLayout->addWidget(endRowSpinBox, 2, 1);
    gridLayout->addWidget(new QLabel(QStringLiteral("结束列"), &dialog), 3, 0);
    gridLayout->addWidget(endColumnSpinBox, 3, 1);
    gridLayout->addWidget(new QLabel(QStringLiteral("bin值位数"), &dialog), 4, 0);
    gridLayout->addWidget(binCharCountComboBox, 4, 1);
    mainLayout->addLayout(gridLayout);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确认"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    if (dialog.exec() != QDialog::Accepted)
    {
        return false;
    }

    m_loadStartRow = qMin(startRowSpinBox->value(), endRowSpinBox->value()) + 1;
    m_loadEndRow = qMax(startRowSpinBox->value(), endRowSpinBox->value()) + 1;
    m_loadStartColumn = qMin(startColumnSpinBox->value(), endColumnSpinBox->value()) + 1;
    m_loadEndColumn = qMax(startColumnSpinBox->value(), endColumnSpinBox->value()) + 1;
    m_loadBinCharCount = binCharCountComboBox->currentIndex() + 1;
    m_hasLoadRange = true;
    return true;
}

// 弹出CSV加载设置窗口，记录X/Y/bin所在列和数据起始行。
bool MapTest::configureCsvLoadOptions(const QString& filePath)
{
    MapLoadRangeDefaults defaults = detectMapLoadRangeDefaults(filePath);

    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("csv加载设置"));
    dialog.setModal(true);
    dialog.setMinimumWidth(360);

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);
    QLabel* fileLabel = new QLabel(QFileInfo(filePath).fileName(), &dialog);
    fileLabel->setWordWrap(true);
    mainLayout->addWidget(fileLabel);

    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->setHorizontalSpacing(12);
    gridLayout->setVerticalSpacing(10);

    QSpinBox* dataStartRowSpinBox = new QSpinBox(&dialog);
    QSpinBox* binColumnSpinBox = new QSpinBox(&dialog);
    QSpinBox* xColumnSpinBox = new QSpinBox(&dialog);
    QSpinBox* yColumnSpinBox = new QSpinBox(&dialog);

    dataStartRowSpinBox->setRange(0, qMax(0, defaults.totalRows - 1));
    binColumnSpinBox->setRange(0, 999);
    xColumnSpinBox->setRange(0, 999);
    yColumnSpinBox->setRange(0, 999);

    dataStartRowSpinBox->setValue(qMax(0, m_csvDataStartRow - 1));
    binColumnSpinBox->setValue(m_csvBinColumn);
    xColumnSpinBox->setValue(m_csvXColumn);
    yColumnSpinBox->setValue(m_csvYColumn);

    gridLayout->addWidget(new QLabel(QStringLiteral("数据起始行"), &dialog), 0, 0);
    gridLayout->addWidget(dataStartRowSpinBox, 0, 1);
    gridLayout->addWidget(new QLabel(QStringLiteral("bin列"), &dialog), 1, 0);
    gridLayout->addWidget(binColumnSpinBox, 1, 1);
    gridLayout->addWidget(new QLabel(QStringLiteral("X坐标列"), &dialog), 2, 0);
    gridLayout->addWidget(xColumnSpinBox, 2, 1);
    gridLayout->addWidget(new QLabel(QStringLiteral("Y坐标列"), &dialog), 3, 0);
    gridLayout->addWidget(yColumnSpinBox, 3, 1);
    mainLayout->addLayout(gridLayout);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确认"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    if (dialog.exec() != QDialog::Accepted)
    {
        return false;
    }

    m_csvDataStartRow = dataStartRowSpinBox->value() + 1;
    m_csvBinColumn = binColumnSpinBox->value();
    m_csvXColumn = xColumnSpinBox->value();
    m_csvYColumn = yColumnSpinBox->value();
    return true;
}

// 弹出Excel加载设置窗口，记录页索引和要读取的单元格范围。
bool MapTest::configureExcelLoadRange(const QString& filePath)
{
    MapLoadRangeDefaults defaults = isXlsxFilePath(filePath)
            ? detectExcelLoadRangeDefaults(filePath, m_excelPageIndex)
            : detectMapLoadRangeDefaults(filePath);

    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("Excel加载设置"));
    dialog.setModal(true);
    dialog.setMinimumWidth(360);

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);
    QLabel* fileLabel = new QLabel(QFileInfo(filePath).fileName(), &dialog);
    fileLabel->setWordWrap(true);
    mainLayout->addWidget(fileLabel);

    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->setHorizontalSpacing(12);
    gridLayout->setVerticalSpacing(10);

    QSpinBox* pageIndexSpinBox = new QSpinBox(&dialog);
    QSpinBox* startRowSpinBox = new QSpinBox(&dialog);
    QSpinBox* startColumnSpinBox = new QSpinBox(&dialog);
    QSpinBox* endRowSpinBox = new QSpinBox(&dialog);
    QSpinBox* endColumnSpinBox = new QSpinBox(&dialog);

    pageIndexSpinBox->setRange(0, 999);
    startRowSpinBox->setRange(0, qMax(0, defaults.totalRows - 1));
    endRowSpinBox->setRange(0, qMax(0, defaults.totalRows - 1));
    startColumnSpinBox->setRange(0, qMax(0, defaults.maxColumns - 1));
    endColumnSpinBox->setRange(0, qMax(0, defaults.maxColumns - 1));

    pageIndexSpinBox->setValue(m_excelPageIndex);
    startRowSpinBox->setValue(qMax(0, defaults.startRow - 1));
    endRowSpinBox->setValue(qMax(0, defaults.endRow - 1));
    startColumnSpinBox->setValue(qMax(0, defaults.startColumn - 1));
    endColumnSpinBox->setValue(qMax(0, defaults.endColumn - 1));

    gridLayout->addWidget(new QLabel(QStringLiteral("页索引"), &dialog), 0, 0);
    gridLayout->addWidget(pageIndexSpinBox, 0, 1);
    gridLayout->addWidget(new QLabel(QStringLiteral("起始行"), &dialog), 1, 0);
    gridLayout->addWidget(startRowSpinBox, 1, 1);
    gridLayout->addWidget(new QLabel(QStringLiteral("起始列"), &dialog), 2, 0);
    gridLayout->addWidget(startColumnSpinBox, 2, 1);
    gridLayout->addWidget(new QLabel(QStringLiteral("终止行"), &dialog), 3, 0);
    gridLayout->addWidget(endRowSpinBox, 3, 1);
    gridLayout->addWidget(new QLabel(QStringLiteral("结束列"), &dialog), 4, 0);
    gridLayout->addWidget(endColumnSpinBox, 4, 1);
    mainLayout->addLayout(gridLayout);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确认"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    if (dialog.exec() != QDialog::Accepted)
    {
        return false;
    }

    m_excelPageIndex = pageIndexSpinBox->value();
    m_loadStartRow = qMin(startRowSpinBox->value(), endRowSpinBox->value()) + 1;
    m_loadEndRow = qMax(startRowSpinBox->value(), endRowSpinBox->value()) + 1;
    m_loadStartColumn = qMin(startColumnSpinBox->value(), endColumnSpinBox->value()) + 1;
    m_loadEndColumn = qMax(startColumnSpinBox->value(), endColumnSpinBox->value()) + 1;
    m_loadBinCharCount = 1;
    m_hasLoadRange = true;
    return true;
}

// 地图读取完成后刷新bin表格、默认起点、刻度尺和大小图。
void MapTest::finishLoadedMapData(const std::unordered_map<std::string, int>& stringCounts)
{
    m_processedSourceCells.clear();
    m_processedOriginalValues.clear();
    m_processedCellText = chooseProcessedCellText(stringCounts);

    std::vector<std::string> stringTypes;
    for (const auto& entry : stringCounts)
    {
        stringTypes.push_back(entry.first);
    }

    int EffectiveNum = 0;
    std::string defaultTargetValue;
    int defaultTargetCount = -1;
    for (const std::string& type : stringTypes)
    {
        if (type == " ")
        {
            continue;
        }
        int count = stringCounts.at(type);
        if (count > defaultTargetCount)
        {
            defaultTargetCount = count;
            defaultTargetValue = type;
        }
    }

    ui->tableWidget->setRowCount(0);
    ui->tableWidget->setEditTriggers(QTableWidget::NoEditTriggers);
    for (int j = 0; j < (int)stringTypes.size(); j++)
    {
        QColor clr(rand() % 256, rand() % 256, rand() % 256);
        ui->tableWidget->setRowCount(EffectiveNum + 1);
        setTableCheckBox(ui->tableWidget, EffectiveNum, 0, stringTypes[j] == defaultTargetValue);
        ui->tableWidget->setItem(EffectiveNum, 1, new QTableWidgetItem(QString::fromStdString(stringTypes[j])));
        ui->tableWidget->setItem(EffectiveNum, 2, new QTableWidgetItem(QString::number(stringCounts.at(stringTypes[j]))));
        ui->tableWidget->setItem(EffectiveNum, 3, new QTableWidgetItem(QString::number(stringCounts.at(stringTypes[j]))));
        ui->tableWidget->setItem(EffectiveNum, 4, new QTableWidgetItem(clr.name()));
        ui->tableWidget->item(EffectiveNum, 4)->setBackground(clr);
        EffectiveNum++;
    }

    ui->tableWidget->setRowCount(EffectiveNum + 1);
    for (int col = 0; col < ui->tableWidget->columnCount(); ++col)
    {
        QWidget* w = ui->tableWidget->cellWidget(EffectiveNum, col);
        if (w)
        {
            ui->tableWidget->removeCellWidget(EffectiveNum, col);
            delete w;
        }
    }

    ui->tableWidget->setItem(EffectiveNum, 0, new QTableWidgetItem(ProcessedCellText));
    ui->tableWidget->setItem(EffectiveNum, 1, new QTableWidgetItem(m_processedCellText));
    ui->tableWidget->setItem(EffectiveNum, 2, new QTableWidgetItem("0"));
    ui->tableWidget->setItem(EffectiveNum, 3, new QTableWidgetItem("0"));
    ui->tableWidget->setItem(EffectiveNum, 4, new QTableWidgetItem("#ffffff"));
    ui->tableWidget->item(EffectiveNum, 4)->setBackground(ProcessedCellColor);

    map_columns = m_mapData[0].size();
    map_rows = m_mapData.count();
    rodegrees = 0;
    rebuildRotatedMap();

    syncMapWidgetGeometry();

    m_scaleFactor = 1;
    RecordCurrent_x = clampToRange(map_columns / 2, 0, qMax(0, map_columns - 1));
    RecordCurrent_y = clampToRange(map_rows / 2, 0, qMax(0, map_rows - 1));
    m_hasStartPoint = false;
    m_hasReferencePoint = false;
    m_hasFoundStartPoint = false;
    m_isFindingStartPoint = false;
    m_verifyPointsSource.clear();
    m_hasEditRectStart = false;
    m_hasEditRectEnd = false;
    m_editSelectionMode = MapEditPoint;
    m_pendingPathDisplayCells.clear();
    m_assistPathSourceCells.clear();
    clearFindStartRoute();
    invalidateMapColorImage();
    ui->edit_point_radioButton->setChecked(true);

    if (!defaultTargetValue.empty())
    {
        std::vector<std::string> defaultTargets;
        defaultTargets.push_back(defaultTargetValue);
        std::pair<int, int> firstTarget = findFirstTargetPosition(currentMapData(), defaultTargets, m_selectedDirection);
        if (firstTarget.first >= 0 && firstTarget.second >= 0)
        {
            RecordCurrent_y = firstTarget.first;
            RecordCurrent_x = firstTarget.second;
            m_startPointSource = displayToSourceCell(RecordCurrent_x, RecordCurrent_y);
            m_hasStartPoint = true;
        }
    }

    centerViewOnCell(RecordCurrent_x, RecordCurrent_y);
    refreshMapViews();
}

// 按CSV列配置读取X/Y/bin数据，并转换成内部地图矩阵。
bool MapTest::loadCsvMapTxtFile()
{
    QFile file(filTexyename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return false;
    }

    struct CsvDie
    {
        int x;
        int y;
        std::string bin;
    };

    QVector<CsvDie> records;
    int minX = INT_MAX;
    int minY = INT_MAX;
    int maxX = INT_MIN;
    int maxY = INT_MIN;
    int lineNumber = 0;

    while (!file.atEnd())
    {
        QByteArray rawLine = file.readLine();
        if (lineNumber++ < m_csvDataStartRow)
        {
            continue;
        }

        QStringList parts = splitMapDataLine(rawLine);
        int maxColumn = qMax(m_csvBinColumn, qMax(m_csvXColumn, m_csvYColumn));
        if (parts.size() <= maxColumn)
        {
            continue;
        }

        bool okX = false;
        bool okY = false;
        int x = parts.at(m_csvXColumn).toInt(&okX);
        int y = parts.at(m_csvYColumn).toInt(&okY);
        QString bin = parts.at(m_csvBinColumn).trimmed();
        if (!okX || !okY || bin.isEmpty())
        {
            continue;
        }

        records.append({x, y, bin.toStdString()});
        minX = qMin(minX, x);
        minY = qMin(minY, y);
        maxX = qMax(maxX, x);
        maxY = qMax(maxY, y);
    }

    if (records.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("csv加载"), QStringLiteral("没有解析到有效的X/Y/bin数据。"));
        return false;
    }

    m_mapData.clear();
    std::unordered_map<std::string, int> stringCounts;
    int rows = maxY - minY + 1;
    int columns = maxX - minX + 1;
    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < columns; ++col)
        {
            m_mapData[row][col] = " ";
        }
    }

    for (const CsvDie& record : records)
    {
        int row = record.y - minY;
        int col = record.x - minX;
        m_mapData[row][col] = record.bin;
        stringCounts[record.bin]++;
    }

    finishLoadedMapData(stringCounts);
    return true;
}

// 直接解析xlsx内部XML，按设置的单元格范围读取地图数据。
bool MapTest::loadExcelMapFile()
{
    if (!isXlsxFilePath(filTexyename))
    {
        QMessageBox::warning(this, QStringLiteral("Excel加载"), QStringLiteral("当前只支持 .xlsx/.xlsm/.xltx 格式。"));
        return false;
    }

    QZipReader zipReader(filTexyename);
    if (!zipReader.exists())
    {
        QMessageBox::warning(this, QStringLiteral("Excel加载"), QStringLiteral("无法打开Excel文件。"));
        return false;
    }

    QString sheetPath = resolveXlsxWorksheetPath(zipReader, m_excelPageIndex);
    QByteArray sheetXml = sheetPath.isEmpty() ? QByteArray() : zipReader.fileData(sheetPath);
    if (sheetXml.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("Excel加载"), QStringLiteral("没有找到对应页索引的工作表。"));
        return false;
    }

    if (!m_hasLoadRange)
    {
        MapLoadRangeDefaults defaults = detectExcelLoadRangeDefaults(filTexyename, m_excelPageIndex);
        m_loadStartRow = defaults.startRow;
        m_loadEndRow = defaults.endRow;
        m_loadStartColumn = defaults.startColumn;
        m_loadEndColumn = defaults.endColumn;
        m_hasLoadRange = true;
    }

    m_loadStartRow = qMax(1, m_loadStartRow);
    m_loadEndRow = qMax(m_loadStartRow, m_loadEndRow);
    m_loadStartColumn = qMax(1, m_loadStartColumn);
    m_loadEndColumn = qMax(m_loadStartColumn, m_loadEndColumn);

    int rows = m_loadEndRow - m_loadStartRow + 1;
    int columns = m_loadEndColumn - m_loadStartColumn + 1;
    if (rows <= 0 || columns <= 0)
    {
        QMessageBox::warning(this, QStringLiteral("Excel加载"), QStringLiteral("Excel加载范围无效。"));
        return false;
    }

    QStringList sharedStrings = readXlsxSharedStrings(zipReader);
    m_mapData.clear();
    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < columns; ++col)
        {
            m_mapData[row][col] = " ";
        }
    }

    QXmlStreamReader reader(sheetXml);
    while (!reader.atEnd())
    {
        reader.readNext();
        if (!reader.isStartElement() || reader.name() != QLatin1String("c"))
        {
            continue;
        }

        XlsxCellRef cell = parseXlsxCellRef(reader.attributes().value(QStringLiteral("r")).toString());
        bool inRange = cell.row >= m_loadStartRow && cell.row <= m_loadEndRow &&
                cell.column >= m_loadStartColumn && cell.column <= m_loadEndColumn;
        if (!inRange)
        {
            reader.skipCurrentElement();
            continue;
        }

        QString cellType = reader.attributes().value(QStringLiteral("t")).toString();
        QString rawValue;
        QString inlineValue;

        while (!reader.atEnd())
        {
            reader.readNext();
            if (reader.isEndElement() && reader.name() == QLatin1String("c"))
            {
                break;
            }
            if (!reader.isStartElement())
            {
                continue;
            }

            if (reader.name() == QLatin1String("v"))
            {
                rawValue = reader.readElementText(QXmlStreamReader::IncludeChildElements);
            }
            else if (reader.name() == QLatin1String("t"))
            {
                inlineValue += reader.readElementText(QXmlStreamReader::IncludeChildElements);
            }
            else if (reader.name() != QLatin1String("is"))
            {
                reader.skipCurrentElement();
            }
        }

        QString value;
        if (cellType == QLatin1String("s"))
        {
            bool ok = false;
            int sharedIndex = rawValue.toInt(&ok);
            value = ok && sharedIndex >= 0 && sharedIndex < sharedStrings.size()
                    ? sharedStrings.at(sharedIndex)
                    : rawValue;
        }
        else if (cellType == QLatin1String("inlineStr"))
        {
            value = inlineValue;
        }
        else
        {
            value = rawValue;
        }

        int rowIndex = cell.row - m_loadStartRow;
        int columnIndex = cell.column - m_loadStartColumn;
        m_mapData[rowIndex][columnIndex] = normalizeXlsxCellText(value).toStdString();
    }

    std::unordered_map<std::string, int> stringCounts;
    int nonEmptyCount = 0;
    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < columns; ++col)
        {
            const std::string& value = mapCellValue(m_mapData, row, col);
            stringCounts[value]++;
            if (value != " ")
            {
                nonEmptyCount++;
            }
        }
    }

    if (nonEmptyCount == 0)
    {
        QMessageBox::warning(this, QStringLiteral("Excel加载"), QStringLiteral("选择范围内没有有效地图数据。"));
        return false;
    }

    finishLoadedMapData(stringCounts);
    return true;
}

//计算出现频率最高的列数
QPair<int, int> findMostFrequentLineRange(const QString& filePath, int& mostFrequentLength) {
    QMap<int, int> lengthCounts;
        QMap<int, int> lengthFirstLine;
        QMap<int, int> lengthLastLine;

        // 打开文件
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return qMakePair(-1, -1);
        }

        // 读取文件并统计每行字符的数量
        QTextStream in(&file);
        int lineNumber = 0;
        while (!in.atEnd()) {
            QString line = in.readLine();
            int length = line.length();
            if (!lengthFirstLine.contains(length))
                lengthFirstLine[length] = lineNumber + 1; // 行号从1开始计数
            lengthCounts[length] += 1;
            lengthLastLine[length] = lineNumber + 1; // 行号从1开始计数
            lineNumber++;
        }

        file.close();

        // 找到出现频率最高的字符数量
        int maxFrequency = 0;
        mostFrequentLength = -1;
        QMap<int, int>::const_iterator it;
        for (it = lengthCounts.constBegin(); it != lengthCounts.constEnd(); ++it) {
            if (it.value() > maxFrequency) {
                mostFrequentLength = it.key();
                maxFrequency = it.value();
            }
        }

        // 返回第一次出现和最后一次出现的行号
        return qMakePair(lengthFirstLine.value(mostFrequentLength, -1), lengthLastLine.value(mostFrequentLength, -1));
    }

//读txt
void MapTest::readMapTextFile()
{
    if (m_activeLoadMode == CsvLoadMode)
    {
        loadCsvMapTxtFile();
        return;
    }
    if (m_activeLoadMode == ExcelLoadMode)
    {
        loadExcelMapFile();
        return;
    }

    QVector<QByteArray> waferRows;
    MapLoadRangeDefaults waferDefaults;
    bool isWaferMapFile = readWaferMapRows(filTexyename, waferRows, &waferDefaults);

    QFile file(filTexyename);
    if (!isWaferMapFile && !file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }

        MapLoadRangeDefaults defaults = isWaferMapFile ? waferDefaults : detectMapLoadRangeDefaults(filTexyename);
        int startRow = defaults.startRow;
        int endRow = defaults.endRow;
        int startColumn = defaults.startColumn;
        int endColumn = defaults.endColumn;

        if (m_hasLoadRange)
        {
            startRow = m_loadStartRow;
            endRow = m_loadEndRow;
            startColumn = m_loadStartColumn;
            endColumn = m_loadEndColumn;
        }
        else
        {
            int mostFrequentColumn = 0;
            if (!isWaferMapFile)
            {
                QPair<int, int> result = findMostFrequentLineRange(filTexyename, mostFrequentColumn);
                if (result.first > 0 && result.second >= result.first)
                {
                    startRow = result.first;
                    endRow = result.second;
                }
            }
        }

        startRow = clampToRange(startRow, 1, defaults.totalRows);
        endRow = clampToRange(endRow, startRow, defaults.totalRows);
        startColumn = clampToRange(startColumn, 1, defaults.maxColumns);
        endColumn = clampToRange(endColumn, startColumn, defaults.maxColumns);

    QVector<QByteArray> mapData;

    if (isWaferMapFile)
    {
        for (int currentLine = startRow; currentLine <= endRow && currentLine <= waferRows.size(); ++currentLine)
        {
            QByteArray line = waferRows.at(currentLine - 1);
            if (m_hasLoadRange)
            {
                if (line.size() < endColumn)
                {
                    line.append(QByteArray(endColumn - line.size(), ' '));
                }
                mapData.append(line.mid(startColumn - 1, endColumn - startColumn + 1));
            }
            else
            {
                mapData.append(line);
            }
        }
    }
    else
    {
        int currentLine = 1;
        while (!file.atEnd() && currentLine <= endRow) {
            QByteArray line = file.readLine();
            if (currentLine >= startRow) {
                if (m_hasLoadRange)
                {
                    line = trimmedLineBreaks(line);
                    if (line.size() < endColumn)
                    {
                        line.append(QByteArray(endColumn - line.size(), ' '));
                    }
                    mapData.append(line.mid(startColumn - 1, endColumn - startColumn + 1));
                }
                else
                {
                    mapData.append(line);
                }
            }
            ++currentLine;
        }
    }

    if (mapData.isEmpty())
    {
        return;
    }


    std::string rowString;
    // 将行数据转换为 QString
    for (const auto& ch : mapData[mapData.size() / 2])
    {
        QString qstr = QString::fromStdString(std::string(1, ch));
        rowString += qstr.toStdString();
    }

    if (!m_hasLoadRange && hasSpacePattern(rowString))
    {
        removeLinesWithoutSpacePattern(mapData);

        int findFirstAllSpaces=findFirstAllSpacesColumn(mapData);
        removeColumns(mapData,findFirstAllSpaces);

        removeWhitespaceRowsAndColumns(mapData);
    }
    else
    {
        if(!m_hasLoadRange && hasSpace(rowString))
        {
            int startIndex = getLastColumnStartIndex(mapData);
            removeRows(mapData,startIndex);

            int findFirstAllSpaces=findFirstAllSpacesColumn(mapData);
            removeColumns(mapData,findFirstAllSpaces);

            removeWhitespaceRowsAndColumns(mapData);

        }
    }

    int charnum = m_hasLoadRange ? clampToRange(m_loadBinCharCount, 1, MaxLoadBinCharCount) : 1;
    if (!m_hasLoadRange)
    {
        // 调用函数找到符合条件的子串并存储在容器中
        std::vector<std::string> results = findAndStore(rowString);

        // 打印容器中的结果
        if (!results.empty() && results[0].length() > 1 && results[0].length() <= 8)
        {
            charnum=results[0].length();
        }
    }

    std::unordered_map<std::string, int> m_stringCount;  // 用于统计字符串数量的哈希表
    int insertIndex = 0;  // 用于记录插入位置
    for (int i = 0; i < mapData.size(); i++)
	{
        for (int j = 0; j < mapData.at(i).size(); j++)
		{

            if(mapData.at(i).at(j)!='\n')
            {
                // 将 QByteArray 转换为 QString
               std::string stringValue = std::string(1, mapData.at(i).at(j));

                // 将字符按照随机长度组合成字符串
               for (int k = 1; k < charnum && j + k < mapData.at(i).size(); k++)
               {
                   stringValue += mapData.at(i).at(j + k);
               }

                // 将数据插入到 m_mapData
                m_mapData[i][insertIndex] = stringValue;

                insertIndex++;
                // 统计字符串数量
                m_stringCount[stringValue]++;

                // 跳过已经处理过的字符
                j += charnum -1;
            }
		}
        insertIndex = 0;
	}

    if (m_mapData.isEmpty() || m_mapData.value(0).isEmpty())
    {
        return;
    }

    m_processedSourceCells.clear();
    m_processedOriginalValues.clear();
    m_processedCellText = chooseProcessedCellText(m_stringCount);

    // 获取 m_mapData 中所有不同字符串的种类

    std::vector<std::string> stringTypes;

    for (const auto& entry : m_stringCount)
    {
        stringTypes.push_back(entry.first);
    }

	int EffectiveNum = 0;//得到包含的字符数
    std::string defaultTargetValue;
    int defaultTargetCount = -1;
    for (const std::string& type : stringTypes)
    {
        int count = m_stringCount[type];
        if (count > defaultTargetCount)
        {
            defaultTargetCount = count;
            defaultTargetValue = type;
        }
    }

    ui->tableWidget->setRowCount(0);
    ui->tableWidget->setEditTriggers(QTableWidget::NoEditTriggers);//不能编辑
    for (int j = 0; j < (int)stringTypes.size(); j++)
    {
            QColor clr(rand() % 256, rand() % 256, rand() % 256);//获取随机颜

            ui->tableWidget->setRowCount(EffectiveNum+1);//新增一行

            setTableCheckBox(ui->tableWidget, EffectiveNum, 0, stringTypes[j] == defaultTargetValue);

            ui->tableWidget->setItem(EffectiveNum, 1, new QTableWidgetItem(QString::fromStdString(stringTypes[j]))); //在新建行插入ascll

            ui->tableWidget->setItem(EffectiveNum, 2, new QTableWidgetItem(QString::number(m_stringCount[stringTypes[j]]))); //在新建行插入 某个ascll数量
            ui->tableWidget->setItem(EffectiveNum, 3, new QTableWidgetItem(QString::number(m_stringCount[stringTypes[j]])));

            ui->tableWidget->setItem(EffectiveNum, 4, new QTableWidgetItem(clr.name())); //设置新建行的背景色
            ui->tableWidget->item(EffectiveNum, 4)->setBackground(clr);

			EffectiveNum++;

    }
    //设置已加工
    ui->tableWidget->setRowCount(EffectiveNum+1);//新增一行

    // 确保新行没有残留的 cell widget（例如复选框），以便显示文本
    for (int col = 0; col < ui->tableWidget->columnCount(); ++col) {
        QWidget* w = ui->tableWidget->cellWidget(EffectiveNum, col);
        if (w) {
            ui->tableWidget->removeCellWidget(EffectiveNum, col);
            delete w;
        }
    }

    ui->tableWidget->setItem(EffectiveNum, 0, new QTableWidgetItem(ProcessedCellText));

    ui->tableWidget->setItem(EffectiveNum, 1, new QTableWidgetItem(m_processedCellText)); //已加工写入地图的符号

    ui->tableWidget->setItem(EffectiveNum, 2, new QTableWidgetItem("0")); //已加工数量由坐标集合统计，不占用地图字符
    ui->tableWidget->setItem(EffectiveNum, 3, new QTableWidgetItem("0"));

    ui->tableWidget->setItem(EffectiveNum, 4, new QTableWidgetItem("#ffffff")); //设置新建行的背景色

    ui->tableWidget->item(EffectiveNum, 4)->setBackground(ProcessedCellColor);


    map_columns = m_mapData[0].size();

	map_rows = m_mapData.count();
    rodegrees = 0;
    rebuildRotatedMap();


    syncMapWidgetGeometry();

    m_scaleFactor=1;
    RecordCurrent_x = clampToRange(map_columns / 2, 0, qMax(0, map_columns - 1));
    RecordCurrent_y = clampToRange(map_rows / 2, 0, qMax(0, map_rows - 1));
    m_hasStartPoint = false;
    m_hasReferencePoint = false;
    m_hasFoundStartPoint = false;
    m_isFindingStartPoint = false;
    m_verifyPointsSource.clear();
    m_hasEditRectStart = false;
    m_hasEditRectEnd = false;
    m_editSelectionMode = MapEditPoint;
    m_pendingPathDisplayCells.clear();
    m_assistPathSourceCells.clear();
    clearFindStartRoute();
    invalidateMapColorImage();
    ui->edit_point_radioButton->setChecked(true);

    if (!defaultTargetValue.empty())
    {
        std::vector<std::string> defaultTargets;
        defaultTargets.push_back(defaultTargetValue);
        std::pair<int, int> firstTarget = findFirstTargetPosition(currentMapData(), defaultTargets, m_selectedDirection);
        if (firstTarget.first >= 0 && firstTarget.second >= 0)
        {
            RecordCurrent_y = firstTarget.first;
            RecordCurrent_x = firstTarget.second;
            m_startPointSource = displayToSourceCell(RecordCurrent_x, RecordCurrent_y);
            m_hasStartPoint = true;
        }
    }

    centerViewOnCell(RecordCurrent_x, RecordCurrent_y);
    refreshMapViews();

}

//事件过滤器
bool MapTest::eventFilter(QObject* watched, QEvent* event)
{
    if ((watched == ui->main_windows_label ||
         watched == ui->small_label ||
         watched == ui->big_map_module ||
         watched == ui->small_map_module) &&
        event->type() == QEvent::Resize)
    {
        syncMapWidgetGeometry();
        if (!m_mapData.isEmpty())
        {
            refreshMapViews();
        }
        return QWidget::eventFilter(watched, event);
    }

    if (watched == ui->main_windows_label)
    {
        if (m_mapData.isEmpty())
        {
            return QWidget::eventFilter(watched, event);
        }

        if (m_pTimer->isActive())
        {
            if (event->type() == QEvent::MouseButtonPress ||
                event->type() == QEvent::MouseButtonRelease ||
                event->type() == QEvent::MouseButtonDblClick ||
                event->type() == QEvent::MouseMove ||
                event->type() == QEvent::Wheel)
            {
                m_isDragging = false;
                ui->main_windows_label->releaseMouse();
                ui->main_windows_label->setCursor(Qt::ArrowCursor);
                return true;
            }
        }

        if (event->type() == QEvent::Wheel)
        {
            QWheelEvent* wheel = static_cast<QWheelEvent*>(event);
            zoomMainMap(wheel->pos(), wheel->angleDelta().y() > 0 ? 1.2 : 1.0 / 1.2);
            return true;
        }

        if (event->type() == QEvent::MouseButtonDblClick)
        {
            QMouseEvent* mouse = static_cast<QMouseEvent*>(event);
            if (mouse->button() == Qt::LeftButton)
            {
                QPoint cell = cellFromMainPoint(mouse->pos());
                setSelectedCell(cell.x(), cell.y(), true);
            }
            return true;
        }

        if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* mouse = static_cast<QMouseEvent*>(event);
            if (mouse->button() == Qt::LeftButton)
            {
                m_isDragging = true;
                m_dragMoved = false;
                m_dragStartPosition = mouse->pos();
                m_lastDragPosition = mouse->pos();
                ui->main_windows_label->setCursor(Qt::ClosedHandCursor);
                ui->main_windows_label->grabMouse();
                return true;
            }
            if (mouse->button() == Qt::RightButton)
            {
                QPoint cell = cellFromMainPoint(mouse->pos());
                setSelectedCell(cell.x(), cell.y(), false);

                QMenu menu(this);
                QAction* setStartAction = menu.addAction(QStringLiteral("设置起点"));
                QAction* setReferenceAction = menu.addAction(QStringLiteral("设置参考点"));
                QAction* addVerifyAction = menu.addAction(QStringLiteral("添加验证点"));
                QAction* clearVerifyAction = menu.addAction(QStringLiteral("清空验证点"));
                QAction* findStartAction = menu.addAction(QStringLiteral("寻起点"));
                menu.addSeparator();
                QAction* resetViewAction = menu.addAction(QStringLiteral("恢复视图"));

                QAction* selectedAction = menu.exec(mouse->globalPos());
                if (selectedAction == setStartAction)
                {
                    setStartPointFromSelection();
                }
                else if (selectedAction == setReferenceAction)
                {
                    setReferencePointFromSelection();
                }
                else if (selectedAction == addVerifyAction)
                {
                    addVerifyPointFromSelection();
                }
                else if (selectedAction == clearVerifyAction)
                {
                    clearVerifyPoints();
                }
                else if (selectedAction == findStartAction)
                {
                    findStartPointRoute();
                }
                else if (selectedAction == resetViewAction)
                {
                    resetMapScale();
                }
                return true;
            }
        }

        if (event->type() == QEvent::MouseMove)
        {
            QMouseEvent* mouse = static_cast<QMouseEvent*>(event);
            if (m_isDragging)
            {
                QPoint delta = mouse->pos() - m_lastDragPosition;
                if (!delta.isNull())
                {
                    panMainMap(delta);
                    m_dragMoved = m_dragMoved || ((mouse->pos() - m_dragStartPosition).manhattanLength() > 3);
                    m_lastDragPosition = mouse->pos();
                }
                return true;
            }
        }

        if (event->type() == QEvent::MouseButtonRelease)
        {
            QMouseEvent* mouse = static_cast<QMouseEvent*>(event);
            if (mouse->button() == Qt::LeftButton)
            {
                ui->main_windows_label->setCursor(Qt::ArrowCursor);
                ui->main_windows_label->releaseMouse();
                m_isDragging = false;
                if (!m_dragMoved)
                {
                    QPoint cell = cellFromMainPoint(mouse->pos());
                    setSelectedCell(cell.x(), cell.y(), false);
                }
                return true;
            }
        }
    }

    if (watched == ui->small_label)
    {
        if (m_mapData.isEmpty())
        {
            return QWidget::eventFilter(watched, event);
        }

        if (m_pTimer->isActive())
        {
            if (event->type() == QEvent::MouseButtonPress ||
                event->type() == QEvent::MouseButtonRelease ||
                event->type() == QEvent::MouseButtonDblClick ||
                event->type() == QEvent::MouseMove ||
                event->type() == QEvent::Wheel)
            {
                return true;
            }
        }

        if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* mouse = static_cast<QMouseEvent*>(event);
            if (mouse->button() == Qt::LeftButton)
            {
                QPoint cell = cellFromSmallPoint(mouse->pos());
                setSelectedCell(cell.x(), cell.y(), true);
                return true;
            }
            if (mouse->button() == Qt::RightButton)
            {
                QPoint cell = cellFromSmallPoint(mouse->pos());
                setSelectedCell(cell.x(), cell.y(), true, false);

                QMenu menu(this);
                QAction* setStartAction = menu.addAction(QStringLiteral("设置起点"));
                QAction* setReferenceAction = menu.addAction(QStringLiteral("设置参考点"));
                QAction* addVerifyAction = menu.addAction(QStringLiteral("添加验证点"));
                QAction* clearVerifyAction = menu.addAction(QStringLiteral("清空验证点"));
                QAction* findStartAction = menu.addAction(QStringLiteral("寻起点"));
                menu.addSeparator();
                QAction* resetViewAction = menu.addAction(QStringLiteral("恢复视图"));

                QAction* selectedAction = menu.exec(mouse->globalPos());
                if (selectedAction == setStartAction)
                {
                    setStartPointFromSelection();
                }
                else if (selectedAction == setReferenceAction)
                {
                    setReferencePointFromSelection();
                }
                else if (selectedAction == addVerifyAction)
                {
                    addVerifyPointFromSelection();
                }
                else if (selectedAction == clearVerifyAction)
                {
                    clearVerifyPoints();
                }
                else if (selectedAction == findStartAction)
                {
                    findStartPointRoute();
                }
                else if (selectedAction == resetViewAction)
                {
                    resetMapScale();
                }
                else
                {
                    refreshMapViews();
                }
                return true;
            }
        }
    }

    QLineEdit* editor = dynamic_cast<QLineEdit*>(watched);
    if (editor && event->type() == QEvent::FocusOut)
    {
        // 编辑器失去焦点，执行输入验证
        QString text = editor->text();
        if (text.isEmpty())
        {
            // 输入为空，发出警告或禁止用户离开单元格
            QMessageBox::warning(this, "Warning", "不能设置为空!");
            // 例如，您可以显示一个警告对话框，或者强制重新编辑该单元格
        }
        else if (text.length() > 1)
        {
            // 输入超过一个字符，发出警告或截取第一个字符
            // 例如，您可以显示一个警告对话框，并截取第一个字符
            editor->setText(text.left(1));
        }
    }

	return QWidget::eventFilter(watched, event);

}

// 公共绘制函数
void MapTest::DrawSmallMap(QLabel* targetLabel, int centerX, int centerY, const std::unordered_map<std::string, QColor>& colorMap)
{
    if (m_mapData.isEmpty())
    {
        targetLabel->clear();
        return;
    }

    QPixmap pixmap(targetLabel->width(), targetLabel->height());
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);

    const int cellCount = 15;
    const double cellWidth = targetLabel->width() / (double)cellCount;
    const double cellHeight = targetLabel->height() / (double)cellCount;

    int startX = centerX - cellCount / 2;
    int startY = centerY - cellCount / 2;
    const MapData& mapData = currentMapData();
    const int columns = displayColumns();
    const int rows = displayRows();

    painter.setPen(QPen(QColor(45, 45, 45), 1));

    for (int row = 0; row < cellCount; ++row)
    {
        int mapY = startY + row;
        const auto rowIter = mapData.constFind(mapY);
        const bool rowValid = rowIter != mapData.constEnd();
        for (int col = 0; col < cellCount; ++col)
        {
            int mapX = startX + col;
            QRectF rect(col * cellWidth, row * cellHeight, cellWidth, cellHeight);
            if (mapX < 0 || mapX >= columns || mapY < 0 || mapY >= rows || !rowValid)
            {
                painter.fillRect(rect, QColor(235, 235, 235));
                painter.drawRect(rect);
                continue;
            }

            const auto cellIter = rowIter.value().constFind(mapX);
            const std::string ASCII = (cellIter == rowIter.value().constEnd()) ? std::string(" ") : cellIter.value();
            auto iter = colorMap.find(ASCII);
            bool isFindStartPath = isFindStartPathDisplayCell(mapX, mapY);
            bool isAssistPath = isAssistPathDisplayCell(mapX, mapY);

            if (isFindStartPath) {
                painter.fillRect(rect, QColor(255, 178, 70));
            } else if (isAssistPath) {
                painter.fillRect(rect, QColor(165, 165, 165));
            } else if (iter != colorMap.end()) {
                painter.fillRect(rect, iter->second);
            } else if (ASCII == " ") {
                painter.fillRect(rect, Qt::white);
            } else {
                painter.fillRect(rect, Qt::white);
            }

            if (cellWidth >= 18 && cellHeight >= 14)
            {
                painter.setPen(Qt::black);
                painter.drawText(rect, Qt::AlignCenter, QString::fromStdString(ASCII));
            }
            painter.setPen(QPen(QColor(45, 45, 45), 1));
            painter.drawRect(rect);
        }
    }

    const bool hasEditSelection = (m_editSelectionMode == MapEditRect && m_hasEditRectStart && m_hasEditRectEnd) ||
                                  (m_editSelectionMode == MapEditCircle);
    bool currentIsStartPoint = false;
    if (m_hasStartPoint)
    {
        QPoint startDisplayCell = sourceToDisplayCell(m_startPointSource.x(), m_startPointSource.y());
        currentIsStartPoint = startDisplayCell.x() == centerX && startDisplayCell.y() == centerY;
    }

    if (hasEditSelection)
    {
        drawSmallMapEditSelection(painter, startX, startY, cellWidth, cellHeight);
    }
    else if (!currentIsStartPoint)
    {
        painter.setPen(QPen(Qt::red, 3));
        QRectF centerRect((cellCount / 2) * cellWidth, (cellCount / 2) * cellHeight, cellWidth, cellHeight);
        painter.drawRect(centerRect.adjusted(1, 1, -1, -1));
    }
    drawSmallMapMarkers(painter, startX, startY, cellWidth, cellHeight, cellCount);

    painter.end();
    targetLabel->setPixmap(pixmap);
}

// 返回当前显示方向下的大图列数，90/270度时行列互换。
int MapTest::displayColumns() const
{
    if (rodegrees == 90 || rodegrees == 270)
    {
        return map_rows;
    }
    return map_columns;
}

// 返回当前显示方向下的大图行数，90/270度时行列互换。
int MapTest::displayRows() const
{
    if (rodegrees == 90 || rodegrees == 270)
    {
        return map_columns;
    }
    return map_rows;
}

// 根据当前旋转角度选择原始地图或旋转后的地图用于绘制。
const MapData& MapTest::currentMapData() const
{
    if (rodegrees == 0)
    {
        return m_mapData;
    }
    return rotate_m_mapData;
}

// 标记缓存图失效，下次刷新大图时再按需重建。
void MapTest::invalidateMapColorImage()
{
    m_mapColorImageDirty = true;
}

// 重建一格一像素的地图缓存图，缩放/平移时可直接贴图，避免每帧遍历所有格子绘制。
void MapTest::rebuildMapColorImage(const std::unordered_map<std::string, QColor>& colorMap)
{
    const int columns = displayColumns();
    const int rows = displayRows();
    if (columns <= 0 || rows <= 0 || m_mapData.isEmpty())
    {
        m_mapColorImage = QImage();
        m_mapColorImageDirty = false;
        return;
    }

    const MapData& mapData = currentMapData();
    const std::string blankCell(" ");
    m_mapColorImage = QImage(columns, rows, QImage::Format_RGB32);
    m_mapColorImage.fill(Qt::white);

    for (int row = 0; row < rows; ++row)
    {
        QRgb* scanLine = reinterpret_cast<QRgb*>(m_mapColorImage.scanLine(row));
        const auto rowIter = mapData.constFind(row);
        if (rowIter == mapData.constEnd())
        {
            continue;
        }

        for (int col = 0; col < columns; ++col)
        {
            QColor color;
            if (isFindStartPathDisplayCell(col, row))
            {
                color = QColor(255, 178, 70);
            }
            else if (isAssistPathDisplayCell(col, row))
            {
                color = QColor(165, 165, 165);
            }
            else
            {
                const auto cellIter = rowIter.value().constFind(col);
                const std::string& value = (cellIter == rowIter.value().constEnd()) ? blankCell : cellIter.value();
                const auto colorIter = colorMap.find(value);
                color = (colorIter != colorMap.end()) ? colorIter->second : QColor(Qt::white);
            }
            scanLine[col] = color.rgb();
        }
    }

    m_mapColorImageDirty = false;
}

// 缓存图有效时只改一个显示坐标像素，运行步进时避免整图重建。
bool MapTest::updateMapColorImagePixel(int displayX, int displayY, const QColor& color)
{
    if (m_mapColorImageDirty ||
        m_mapColorImage.isNull() ||
        displayX < 0 || displayY < 0 ||
        displayX >= m_mapColorImage.width() ||
        displayY >= m_mapColorImage.height())
    {
        return false;
    }

    m_mapColorImage.setPixelColor(displayX, displayY, color);
    return true;
}

// 从表格读取bin颜色配置，生成绘图时使用的颜色映射。
std::unordered_map<std::string, QColor> MapTest::buildColorMap() const
{
    std::unordered_map<std::string, QColor> colorMap;
    for (int n = 0; n < ui->tableWidget->rowCount(); n++)
    {
        QString data = ui->tableWidget->model()->index(n, 1).data().toString();
        if (data.isEmpty())
        {
            continue;
        }

        QString colorName = ui->tableWidget->model()->index(n, 4).data().toString();
        QColor color(colorName);
        if (!color.isValid())
        {
            color = Qt::white;
        }
        colorMap[data.toStdString()] = color;
    }
    return colorMap;
}

// 读取表格中勾选的bin，作为启动加工时需要搜索的目标。
std::unordered_map<std::string, int> MapTest::checkedTargetRows() const
{
    std::unordered_map<std::string, int> checkedRows;
    for (int row = 0; row < ui->tableWidget->rowCount(); ++row)
    {
        QWidget* pWidget = ui->tableWidget->cellWidget(row, 0);
        bool checked = false;
        if (pWidget)
        {
            QList<QCheckBox*> boxes = pWidget->findChildren<QCheckBox*>();
            if (!boxes.isEmpty())
            {
                checked = (boxes.first()->checkState() == Qt::Checked);
            }
        }

        if (checked)
        {
            QString data = ui->tableWidget->model()->index(row, 1).data().toString();
            if (!data.isEmpty())
            {
                checkedRows[data.toStdString()] = row;
            }
        }
    }
    return checkedRows;
}

// 选择已加工写入字符：默认#，如果原图已有#，则从常用符号中挑一个未占用字符。
QString MapTest::chooseProcessedCellText(const std::unordered_map<std::string, int>& stringCounts) const
{
    auto containsValue = [&stringCounts](const QString& text) {
        return stringCounts.find(text.toStdString()) != stringCounts.end();
    };

    if (!containsValue(QStringLiteral("#")))
    {
        return QStringLiteral("#");
    }

    QStringList candidates;
    candidates << QStringLiteral("*")
               << QStringLiteral("@")
               << QStringLiteral("$")
               << QStringLiteral("%")
               << QStringLiteral("&")
               << QStringLiteral("!")
               << QStringLiteral("?")
               << QStringLiteral("+")
               << QStringLiteral("=")
               << QStringLiteral("~")
               << QStringLiteral("^")
               << QStringLiteral(";")
               << QStringLiteral(":")
               << QStringLiteral("/")
               << QStringLiteral("\\")
               << QStringLiteral("|");

    int offset = candidates.isEmpty() ? 0 : QRandomGenerator::global()->bounded(candidates.size());
    for (int i = 0; i < candidates.size(); ++i)
    {
        QString candidate = candidates.at((offset + i) % candidates.size());
        if (!containsValue(candidate))
        {
            return candidate;
        }
    }

    for (char ch = 33; ch <= 126; ++ch)
    {
        QString candidate;
        candidate.append(QChar(QLatin1Char(ch)));
        if (!containsValue(candidate))
        {
            return candidate;
        }
    }

    return QStringLiteral("#");
}

// 表格第0列写“已加工”的行是状态行，第1列才是真正写入地图的单字符。
bool MapTest::isProcessedTableRow(int row) const
{
    QTableWidgetItem* titleItem = ui->tableWidget->item(row, 0);
    return titleItem != nullptr && titleItem->text() == ProcessedCellText;
}

// 修改已加工字符：必须是单个非空可见ASCII字符，且不能和普通bin冲突。
bool MapTest::updateProcessedCellText(const QString& text, int row)
{
    if (m_pTimer->isActive())
    {
        return false;
    }

    QString value = text.trimmed().left(1);
    if (value.isEmpty() || value == QStringLiteral(" "))
    {
        QMessageBox::warning(this, QStringLiteral("已加工字符"), QStringLiteral("已加工字符不能为空或空格。"));
        return false;
    }

    for (int tableRow = 0; tableRow < ui->tableWidget->rowCount(); ++tableRow)
    {
        if (isProcessedTableRow(tableRow))
        {
            continue;
        }

        QTableWidgetItem* dataItem = ui->tableWidget->item(tableRow, 1);
        if (dataItem != nullptr && dataItem->text() == value)
        {
            QMessageBox::warning(this,
                                 QStringLiteral("已加工字符"),
                                 QStringLiteral("字符“%1”已经是地图bin，不能作为已加工字符。").arg(value));
            return false;
        }
    }

    std::string stdValue = value.toStdString();
    for (int sourceY = 0; sourceY < map_rows; ++sourceY)
    {
        for (int sourceX = 0; sourceX < map_columns; ++sourceX)
        {
            qint64 key = sourceCellKey(sourceX, sourceY);
            if (!m_processedSourceCells.contains(key) &&
                mapCellValue(m_mapData, sourceY, sourceX) == stdValue)
            {
                QMessageBox::warning(this,
                                     QStringLiteral("已加工字符"),
                                     QStringLiteral("字符“%1”已经存在于地图中，不能作为已加工字符。").arg(value));
                return false;
            }
        }
    }

    m_processedCellText = value;
    if (row >= 0 && row < ui->tableWidget->rowCount())
    {
        QTableWidgetItem* item = ui->tableWidget->item(row, 1);
        if (item == nullptr)
        {
            item = new QTableWidgetItem();
            ui->tableWidget->setItem(row, 1, item);
        }
        item->setText(m_processedCellText);
    }

    for (qint64 key : m_processedSourceCells)
    {
        int sourceX = int(quint32(key));
        int sourceY = int(key >> 32);
        if (sourceX >= 0 && sourceY >= 0 && sourceX < map_columns && sourceY < map_rows)
        {
            m_mapData[sourceY][sourceX] = stdValue;
        }
    }

    rebuildRotatedMap();
    updateTableCountsFromMapData();
    invalidateMapColorImage();
    refreshMapViews();
    return true;
}

// 如果当前格属于勾选目标，则标记为已加工并同步表格计数。
bool MapTest::processCurrentCellIfChecked()
{
    if (m_mapData.isEmpty())
    {
        return false;
    }

    std::unordered_map<std::string, int> checkedRows = checkedTargetRows();
    if (checkedRows.empty())
    {
        return false;
    }

    QPoint sourceCell = displayToSourceCell(RecordCurrent_x, RecordCurrent_y);
    int sourceX = clampToRange(sourceCell.x(), 0, map_columns - 1);
    int sourceY = clampToRange(sourceCell.y(), 0, map_rows - 1);
    std::string currentValue = mapCellValue(m_mapData, sourceY, sourceX);
    auto value = checkedRows.find(currentValue);
    if (value == checkedRows.end())
    {
        return false;
    }

    qint64 key = sourceCellKey(sourceX, sourceY);
    if (m_processedSourceCells.contains(key))
    {
        return false;
    }

    std::string processedValue = m_processedCellText.toStdString();
    if (processedValue.empty())
    {
        return false;
    }

    m_processedSourceCells.insert(key);
    m_processedOriginalValues.insert(key, currentValue);
    m_mapData[sourceY][sourceX] = processedValue;
    if (rodegrees == 90)
    {
        rotate_m_mapData[sourceX][map_rows - 1 - sourceY] = processedValue;
    }
    else if (rodegrees == 180)
    {
        rotate_m_mapData[map_rows - 1 - sourceY][map_columns - 1 - sourceX] = processedValue;
    }
    else if (rodegrees == 270)
    {
        rotate_m_mapData[map_columns - 1 - sourceX][sourceY] = processedValue;
    }

    m_assistPathSourceCells.remove(key);
    std::unordered_map<std::string, QColor> colorMap = buildColorMap();
    auto processedColor = colorMap.find(processedValue);
    QColor displayColor = processedColor != colorMap.end() ? processedColor->second : ProcessedCellColor;
    if (!updateMapColorImagePixel(RecordCurrent_x, RecordCurrent_y, displayColor))
    {
        invalidateMapColorImage();
    }

    QTableWidgetItem* remainingItem = ui->tableWidget->item(value->second, 3);
    if (remainingItem != nullptr)
    {
        remainingItem->setText(QString::number(qMax(0, remainingItem->text().toInt() - 1)));
    }

    for (int row = 0; row < ui->tableWidget->rowCount(); ++row)
    {
        if (!isProcessedTableRow(row))
        {
            continue;
        }

        QTableWidgetItem* totalItem = ui->tableWidget->item(row, 2);
        if (totalItem == nullptr)
        {
            totalItem = new QTableWidgetItem();
            ui->tableWidget->setItem(row, 2, totalItem);
        }
        totalItem->setText(QString::number(m_processedSourceCells.size()));

        QTableWidgetItem* processedRemainingItem = ui->tableWidget->item(row, 3);
        if (processedRemainingItem == nullptr)
        {
            processedRemainingItem = new QTableWidgetItem();
            ui->tableWidget->setItem(row, 3, processedRemainingItem);
        }
        processedRemainingItem->setText(QStringLiteral("0"));
        break;
    }

    ui->tableWidget->viewport()->update();

    return true;
}

// 计算大图当前可见的地图坐标范围。
QRectF MapTest::currentViewRect() const
{
    int cols = displayColumns();
    int rows = displayRows();
    if (cols <= 0 || rows <= 0)
    {
        return QRectF();
    }

    double viewCols = qMax(1.0, cols / m_scaleFactor);
    double viewRows = qMax(1.0, rows / m_scaleFactor);
    viewCols = qMin(viewCols, (double)cols);
    viewRows = qMin(viewRows, (double)rows);

    double left = m_viewCenterCell.x() - viewCols / 2.0;
    double top = m_viewCenterCell.y() - viewRows / 2.0;
    left = clampToRange(left, 0.0, qMax(0.0, cols - viewCols));
    top = clampToRange(top, 0.0, qMax(0.0, rows - viewRows));

    return QRectF(left, top, viewCols, viewRows);
}

// 将大图label里的鼠标像素位置换算成当前显示地图坐标。
QPoint MapTest::cellFromMainPoint(const QPoint& pos) const
{
    QRectF view = currentViewRect();
    if (view.isNull())
    {
        return QPoint(0, 0);
    }

    double x = view.left() + pos.x() * view.width() / qMax(1, ui->main_windows_label->width());
    double y = view.top() + pos.y() * view.height() / qMax(1, ui->main_windows_label->height());
    return QPoint(clampToRange((int)floor(x), 0, displayColumns() - 1),
                  clampToRange((int)floor(y), 0, displayRows() - 1));
}

// 将小图label里的鼠标像素位置换算成当前显示地图坐标。
QPoint MapTest::cellFromSmallPoint(const QPoint& pos) const
{
    const int cellCount = 15;
    double cellWidth = ui->small_label->width() / (double)cellCount;
    double cellHeight = ui->small_label->height() / (double)cellCount;
    int dx = clampToRange((int)floor(pos.x() / qMax(1.0, cellWidth)), 0, cellCount - 1) - cellCount / 2;
    int dy = clampToRange((int)floor(pos.y() / qMax(1.0, cellHeight)), 0, cellCount - 1) - cellCount / 2;
    return QPoint(clampToRange(RecordCurrent_x + dx, 0, displayColumns() - 1),
                  clampToRange(RecordCurrent_y + dy, 0, displayRows() - 1));
}

// 把当前显示坐标转换回未旋转的原始地图坐标。
QPoint MapTest::displayToSourceCell(int x, int y) const
{
    if (rodegrees == 90)
    {
        return QPoint(y, map_rows - 1 - x);
    }
    if (rodegrees == 180)
    {
        return QPoint(map_columns - 1 - x, map_rows - 1 - y);
    }
    if (rodegrees == 270)
    {
        return QPoint(map_columns - 1 - y, x);
    }
    return QPoint(x, y);
}

// 把原始地图坐标转换为当前旋转角度下的显示坐标。
QPoint MapTest::sourceToDisplayCell(int x, int y) const
{
    if (rodegrees == 90)
    {
        return QPoint(map_rows - 1 - y, x);
    }
    if (rodegrees == 180)
    {
        return QPoint(map_columns - 1 - x, map_rows - 1 - y);
    }
    if (rodegrees == 270)
    {
        return QPoint(y, map_columns - 1 - x);
    }
    return QPoint(x, y);
}

// 将整数限制在给定范围内，范围异常时返回下限。
int MapTest::clampToRange(int value, int minValue, int maxValue) const
{
    if (maxValue < minValue)
    {
        return minValue;
    }
    return qBound(minValue, value, maxValue);
}

// 将浮点数限制在给定范围内，范围异常时返回下限。
double MapTest::clampToRange(double value, double minValue, double maxValue) const
{
    if (maxValue < minValue)
    {
        return minValue;
    }
    return qBound(minValue, value, maxValue);
}

// 把原始地图坐标打包成唯一key，用于QSet快速记录借助路径。
qint64 MapTest::sourceCellKey(int x, int y) const
{
    return (qint64(y) << 32) ^ quint32(x);
}

// 判断显示坐标是否已经被标记为借助路径。
bool MapTest::isAssistPathDisplayCell(int displayX, int displayY) const
{
    if (displayX < 0 || displayY < 0 || displayX >= displayColumns() || displayY >= displayRows())
    {
        return false;
    }

    QPoint sourceCell = displayToSourceCell(displayX, displayY);
    if (sourceCell.x() < 0 || sourceCell.y() < 0 ||
        sourceCell.x() >= map_columns || sourceCell.y() >= map_rows)
    {
        return false;
    }
    return m_assistPathSourceCells.contains(sourceCellKey(sourceCell.x(), sourceCell.y()));
}

// 判断显示坐标是否属于寻起点路线。
bool MapTest::isFindStartPathDisplayCell(int displayX, int displayY) const
{
    if (displayX < 0 || displayY < 0 || displayX >= displayColumns() || displayY >= displayRows())
    {
        return false;
    }

    QPoint sourceCell = displayToSourceCell(displayX, displayY);
    if (sourceCell.x() < 0 || sourceCell.y() < 0 ||
        sourceCell.x() >= map_columns || sourceCell.y() >= map_rows)
    {
        return false;
    }
    return m_findStartPathSourceCells.contains(sourceCellKey(sourceCell.x(), sourceCell.y()));
}

// 将显示坐标换算为原始坐标后标记为借助路径。
void MapTest::markAssistPathCell(int displayX, int displayY)
{
    if (displayX < 0 || displayY < 0 || displayX >= displayColumns() || displayY >= displayRows())
    {
        return;
    }

    QPoint sourceCell = displayToSourceCell(displayX, displayY);
    if (sourceCell.x() < 0 || sourceCell.y() < 0 ||
        sourceCell.x() >= map_columns || sourceCell.y() >= map_rows)
    {
        return;
    }
    m_assistPathSourceCells.insert(sourceCellKey(sourceCell.x(), sourceCell.y()));
    if (!updateMapColorImagePixel(displayX, displayY, QColor(165, 165, 165)))
    {
        invalidateMapColorImage();
    }
}

// 将显示坐标换算为原始坐标后标记为寻起点已走路线。
void MapTest::markFindStartPathCell(int displayX, int displayY)
{
    if (displayX < 0 || displayY < 0 || displayX >= displayColumns() || displayY >= displayRows())
    {
        return;
    }

    QPoint sourceCell = displayToSourceCell(displayX, displayY);
    if (sourceCell.x() < 0 || sourceCell.y() < 0 ||
        sourceCell.x() >= map_columns || sourceCell.y() >= map_rows)
    {
        return;
    }
    m_findStartPathSourceCells.insert(sourceCellKey(sourceCell.x(), sourceCell.y()));
    if (!updateMapColorImagePixel(displayX, displayY, QColor(255, 178, 70)))
    {
        invalidateMapColorImage();
    }
}

// 清空寻起点路线并让地图缓存下次重建。
void MapTest::clearFindStartRoute()
{
    if (m_findStartPathSourceCells.isEmpty() && m_pendingFindStartDisplayCells.isEmpty())
    {
        return;
    }

    m_findStartPathSourceCells.clear();
    m_pendingFindStartDisplayCells.clear();
    invalidateMapColorImage();
}

// 更新当前选中格，并按需要居中和刷新大小图。
void MapTest::setSelectedCell(int x, int y, bool centerView, bool refresh)
{
    if (m_mapData.isEmpty())
    {
        return;
    }

    RecordCurrent_x = clampToRange(x, 0, displayColumns() - 1);
    RecordCurrent_y = clampToRange(y, 0, displayRows() - 1);

    if (centerView)
    {
        centerViewOnCell(RecordCurrent_x, RecordCurrent_y);
    }

    updateSelectionUi();

    if (refresh)
    {
        refreshMapViews();
    }
}

// 将当前选中格保存为起点。
void MapTest::setStartPointFromSelection()
{
    setStartPoint(RecordCurrent_x, RecordCurrent_y);
}

// 将当前选中格保存为参考点，参考点只能保留一个。
void MapTest::setReferencePointFromSelection()
{
    setReferencePoint(RecordCurrent_x, RecordCurrent_y);
}

// 将当前选中格追加为验证点，重复点不再重复添加。
void MapTest::addVerifyPointFromSelection()
{
    addVerifyPoint(RecordCurrent_x, RecordCurrent_y);
}

// 清空所有验证点并刷新地图标记。
void MapTest::clearVerifyPoints()
{
    clearVerifyPointList();
}

// 对外接口：按参考点、验证点和起点生成寻起点路径，不自动定时运行。
bool MapTest::prepareFindStartRoute()
{
    if (m_mapData.isEmpty())
    {
        return false;
    }
    if (m_pTimer->isActive() && !m_isFindingStartPoint)
    {
        return false;
    }
    if (m_isFindingStartPoint && !m_pendingFindStartDisplayCells.isEmpty())
    {
        return true;
    }
    if (!m_hasReferencePoint)
    {
        QMessageBox::warning(this, QStringLiteral("寻起点"), QStringLiteral("请先设置参考点。"));
        return false;
    }
    if (!m_hasStartPoint)
    {
        QMessageBox::warning(this, QStringLiteral("寻起点"), QStringLiteral("请先设置起点。"));
        return false;
    }

    QVector<QPoint> routePoints;
    routePoints.append(m_referencePointSource);
    for (const QPoint& verifyPoint : m_verifyPointsSource)
    {
        routePoints.append(verifyPoint);
    }
    routePoints.append(m_startPointSource);

    std::unordered_map<std::string, int> checkedRows = checkedTargetRows();
    std::unordered_set<std::string> targetSet;
    for (const auto& checkedRow : checkedRows)
    {
        targetSet.insert(checkedRow.first);
    }

    auto buildSmartSegment = [this, &targetSet](const QPoint& from, const QPoint& to) {
        QVector<QPoint> path;
        const MapData& mapData = currentMapData();
        const int rows = displayRows();
        const int columns = displayColumns();
        if (rows <= 0 || columns <= 0)
        {
            return path;
        }

        QPoint start(clampToRange(from.x(), 0, columns - 1),
                     clampToRange(from.y(), 0, rows - 1));
        QPoint target(clampToRange(to.x(), 0, columns - 1),
                      clampToRange(to.y(), 0, rows - 1));
        if (start == target)
        {
            return path;
        }

        auto stepCost = [&](int row, int col) {
            if ((row == target.y() && col == target.x()) ||
                (row == start.y() && col == start.x()))
            {
                return 1;
            }

            const std::string& value = mapCellValue(mapData, row, col);
            if (targetSet.find(value) != targetSet.end())
            {
                return 1;
            }
            if (value != " ")
            {
                return targetSet.empty() ? 2 : 7;
            }
            return 20;
        };

        struct QueueNode
        {
            int priority;
            int cost;
            int order;
            int row;
            int col;
            bool operator<(const QueueNode& other) const
            {
                if (priority != other.priority)
                {
                    return priority > other.priority;
                }
                return order > other.order;
            }
        };

        MapDirectionVectors vectors = directionVectors(m_selectedDirection);
        QVector<DirectionVector> neighbors;
        auto addNeighbor = [&neighbors](const DirectionVector& vector) {
            if (vector.dx == 0 && vector.dy == 0)
            {
                return;
            }
            for (const DirectionVector& existing : neighbors)
            {
                if (existing.dx == vector.dx && existing.dy == vector.dy)
                {
                    return;
                }
            }
            neighbors.append(vector);
        };
        addNeighbor(vectors.minor);
        addNeighbor(vectors.major);
        addNeighbor(negateVector(vectors.minor));
        addNeighbor(negateVector(vectors.major));
        addNeighbor(DirectionVector{1, 0});
        addNeighbor(DirectionVector{0, 1});
        addNeighbor(DirectionVector{-1, 0});
        addNeighbor(DirectionVector{0, -1});

        auto heuristic = [&target](int row, int col) {
            return qAbs(target.y() - row) + qAbs(target.x() - col);
        };

        QVector<QVector<int>> distance(rows, QVector<int>(columns, INT_MAX));
        QVector<QVector<QPoint>> parent(rows, QVector<QPoint>(columns, QPoint(-1, -1)));
        std::priority_queue<QueueNode> queue;
        int visitOrder = 0;
        bool found = false;

        distance[start.y()][start.x()] = 0;
        queue.push({heuristic(start.y(), start.x()), 0, visitOrder++, start.y(), start.x()});
        while (!queue.empty() && !found)
        {
            QueueNode current = queue.top();
            queue.pop();
            if (current.cost != distance[current.row][current.col])
            {
                continue;
            }

            for (const DirectionVector& neighbor : neighbors)
            {
                int nextCol = current.col + neighbor.dx;
                int nextRow = current.row + neighbor.dy;
                if (nextCol < 0 || nextCol >= columns || nextRow < 0 || nextRow >= rows)
                {
                    continue;
                }

                int nextCost = current.cost + stepCost(nextRow, nextCol);
                if (nextCost >= distance[nextRow][nextCol])
                {
                    continue;
                }

                distance[nextRow][nextCol] = nextCost;
                parent[nextRow][nextCol] = QPoint(current.col, current.row);
                if (nextRow == target.y() && nextCol == target.x())
                {
                    found = true;
                    break;
                }
                queue.push({nextCost + heuristic(nextRow, nextCol), nextCost, visitOrder++, nextRow, nextCol});
            }
        }

        if (!found)
        {
            return path;
        }

        QVector<QPoint> reversedPath;
        QPoint cursor = target;
        while (cursor != start && cursor.x() >= 0 && cursor.y() >= 0)
        {
            reversedPath.append(cursor);
            cursor = parent[cursor.y()][cursor.x()];
        }
        for (int i = reversedPath.size() - 1; i >= 0; --i)
        {
            path.append(reversedPath.at(i));
        }
        return path;
    };

    QVector<QPoint> pendingPath;
    for (int i = 1; i < routePoints.size(); ++i)
    {
        QPoint from = sourceToDisplayCell(routePoints.at(i - 1).x(), routePoints.at(i - 1).y());
        QPoint to = sourceToDisplayCell(routePoints.at(i).x(), routePoints.at(i).y());
        QVector<QPoint> segment = buildSmartSegment(from, to);
        if (segment.isEmpty() && from != to)
        {
            QMessageBox::warning(this, QStringLiteral("寻起点"), QStringLiteral("无法生成从第%1个点到第%2个点的路线。").arg(i).arg(i + 1));
            return false;
        }
        pendingPath += segment;
    }

    clearFindStartRoute();
    m_pendingFindStartDisplayCells = pendingPath;
    m_hasFoundStartPoint = false;
    m_isFindingStartPoint = true;

    QPoint referenceDisplayCell = sourceToDisplayCell(m_referencePointSource.x(), m_referencePointSource.y());
    markFindStartPathCell(referenceDisplayCell.x(), referenceDisplayCell.y());
    setSelectedCell(referenceDisplayCell.x(), referenceDisplayCell.y(), true, false);
    invalidateMapColorImage();
    refreshMapViews();

    if (m_pendingFindStartDisplayCells.isEmpty())
    {
        m_isFindingStartPoint = false;
        m_hasFoundStartPoint = true;
        ui->label->setText(QStringLiteral("已寻到起点"));
        return true;
    }

    setMapFunctionPanelRunning(true);
    return true;
}

// 对外接口：开始自动寻起点，内部仍然按单步接口逐格推进。
bool MapTest::startFindStartPoint()
{
    if (m_pTimer->isActive())
    {
        return false;
    }

    if (!prepareFindStartRoute())
    {
        return false;
    }

    if (!m_hasFoundStartPoint)
    {
        m_pTimer->start(MapStepIntervalMs);
    }
    return true;
}

// UI包装：按钮和右键菜单调用统一公开接口。
void MapTest::findStartPointRoute()
{
    startFindStartPoint();
}

// 在大图可见区域内绘制起点、参考点和验证点标记。
void MapTest::drawMainMapMarkers(QPainter& painter, const QRectF& view, double cellWidth, double cellHeight)
{
    auto drawCellStart = [&](const QPoint& sourceCell) {
        QPoint displayCell = sourceToDisplayCell(sourceCell.x(), sourceCell.y());
        if (displayCell.x() < view.left() || displayCell.x() >= view.right() ||
            displayCell.y() < view.top() || displayCell.y() >= view.bottom())
        {
            return;
        }

        QPointF center((displayCell.x() + 0.5 - view.left()) * cellWidth,
                       (displayCell.y() + 0.5 - view.top()) * cellHeight);
        double radius = qBound(7.0, qMin(cellWidth, cellHeight) * 0.45, 22.0);
        drawStartMarker(painter, center, radius);
    };

    auto drawCellMarker = [&](const QPoint& sourceCell,
                              const QString& text,
                              const QColor& borderColor,
                              const QColor& fillColor) {
        QPoint displayCell = sourceToDisplayCell(sourceCell.x(), sourceCell.y());
        if (displayCell.x() < view.left() || displayCell.x() >= view.right() ||
            displayCell.y() < view.top() || displayCell.y() >= view.bottom())
        {
            return;
        }

        QPointF tip((displayCell.x() + 0.5 - view.left()) * cellWidth,
                    (displayCell.y() + 0.5 - view.top()) * cellHeight);
        double size = qBound(12.0, qMin(cellWidth, cellHeight) * 0.9, 30.0);
        drawReferenceMarker(painter, tip, size, text, borderColor, fillColor);
    };

    if (m_hasStartPoint)
    {
        drawCellStart(m_startPointSource);
    }

    if (m_hasReferencePoint)
    {
        drawCellMarker(m_referencePointSource,
                       QStringLiteral("参"),
                       QColor(20, 155, 80),
                       QColor(205, 255, 220, 210));
    }

    for (int i = 0; i < m_verifyPointsSource.size(); ++i)
    {
        drawCellMarker(m_verifyPointsSource.at(i),
                       QStringLiteral("验%1").arg(i + 1),
                       QColor(0, 190, 210),
                       QColor(210, 255, 255, 190));
    }
}

// 在小图15格视野内绘制起点、参考点和验证点标记。
void MapTest::drawSmallMapMarkers(QPainter& painter, int startX, int startY, double cellWidth, double cellHeight, int cellCount)
{
    auto visibleCenter = [&](const QPoint& sourceCell, QPointF& center) {
        QPoint displayCell = sourceToDisplayCell(sourceCell.x(), sourceCell.y());
        int localX = displayCell.x() - startX;
        int localY = displayCell.y() - startY;
        if (localX < 0 || localX >= cellCount || localY < 0 || localY >= cellCount)
        {
            return false;
        }
        center = QPointF((localX + 0.5) * cellWidth, (localY + 0.5) * cellHeight);
        return true;
    };

    QPointF center;
    if (m_hasStartPoint && visibleCenter(m_startPointSource, center))
    {
        double radius = qBound(7.0, qMin(cellWidth, cellHeight) * 0.45, 20.0);
        drawStartMarker(painter, center, radius);
    }

    if (m_hasReferencePoint && visibleCenter(m_referencePointSource, center))
    {
        double size = qBound(10.0, qMin(cellWidth, cellHeight) * 0.85, 26.0);
        drawReferenceMarker(painter,
                            center,
                            size,
                            QStringLiteral("参"),
                            QColor(20, 155, 80),
                            QColor(205, 255, 220, 210));
    }

    for (int i = 0; i < m_verifyPointsSource.size(); ++i)
    {
        if (visibleCenter(m_verifyPointsSource.at(i), center))
        {
            double size = qBound(10.0, qMin(cellWidth, cellHeight) * 0.85, 26.0);
            drawReferenceMarker(painter,
                                center,
                                size,
                                QStringLiteral("验%1").arg(i + 1),
                                QColor(0, 190, 210),
                                QColor(210, 255, 255, 190));
        }
    }
}

// 刷新Map编辑区域里当前点、左上点和右下点坐标显示。
void MapTest::updateEditSelectionUi()
{
    auto displayText = [](const QPoint& point, bool valid) {
        if (!valid)
        {
            return QStringLiteral("--, --");
        }
        return QStringLiteral("%1, %2").arg(point.x() + 1).arg(point.y() + 1);
    };

    ui->edit_current_point_value_label->setText(m_mapData.isEmpty()
                                                ? QStringLiteral("--, --")
                                                : QStringLiteral("%1, %2").arg(RecordCurrent_x + 1).arg(RecordCurrent_y + 1));
    ui->edit_left_top_value_label->setText(displayText(m_editRectStartDisplay, m_hasEditRectStart));
    ui->edit_right_bottom_value_label->setText(displayText(m_editRectEndDisplay, m_hasEditRectEnd));
}

// 切换点选、框选或圆选模式，并立即刷新选区显示。
void MapTest::setEditSelectionMode(MapEditSelectionMode mode)
{
    m_editSelectionMode = mode;
    if (mode == MapEditPoint)
    {
        m_hasEditRectStart = false;
        m_hasEditRectEnd = false;
    }
    updateEditSelectionUi();
    refreshMapViews();
}

// 把当前点记录为框选左上点，重新等待右下点。
void MapTest::setEditRectStartFromSelection()
{
    if (m_mapData.isEmpty())
    {
        return;
    }

    m_editRectStartDisplay = QPoint(RecordCurrent_x, RecordCurrent_y);
    m_hasEditRectStart = true;
    m_hasEditRectEnd = false;
    updateEditSelectionUi();
    refreshMapViews();
}

// 把当前点记录为框选右下点。
void MapTest::setEditRectEndFromSelection()
{
    if (m_mapData.isEmpty())
    {
        return;
    }

    m_editRectEndDisplay = QPoint(RecordCurrent_x, RecordCurrent_y);
    m_hasEditRectEnd = true;
    updateEditSelectionUi();
    refreshMapViews();
}

// 根据当前编辑模式收集需要修改的原始地图坐标。
QVector<QPoint> MapTest::editSelectionSourceCells() const
{
    QVector<QPoint> cells;
    if (m_mapData.isEmpty())
    {
        return cells;
    }

    auto appendDisplayCell = [&](int displayX, int displayY) {
        if (displayX < 0 || displayX >= displayColumns() || displayY < 0 || displayY >= displayRows())
        {
            return;
        }
        QPoint sourceCell = displayToSourceCell(displayX, displayY);
        if (sourceCell.x() >= 0 && sourceCell.x() < map_columns &&
            sourceCell.y() >= 0 && sourceCell.y() < map_rows)
        {
            cells.append(sourceCell);
        }
    };

    if (m_editSelectionMode == MapEditPoint)
    {
        appendDisplayCell(RecordCurrent_x, RecordCurrent_y);
        return cells;
    }

    if (m_editSelectionMode == MapEditRect)
    {
        if (!m_hasEditRectStart || !m_hasEditRectEnd)
        {
            appendDisplayCell(RecordCurrent_x, RecordCurrent_y);
            return cells;
        }

        int left = qMin(m_editRectStartDisplay.x(), m_editRectEndDisplay.x());
        int right = qMax(m_editRectStartDisplay.x(), m_editRectEndDisplay.x());
        int top = qMin(m_editRectStartDisplay.y(), m_editRectEndDisplay.y());
        int bottom = qMax(m_editRectStartDisplay.y(), m_editRectEndDisplay.y());
        for (int y = top; y <= bottom; ++y)
        {
            for (int x = left; x <= right; ++x)
            {
                appendDisplayCell(x, y);
            }
        }
        return cells;
    }

    int radius = qMax(1, ui->ring_count_spinBox->value());
    int radiusSquared = radius * radius;
    for (int dy = -radius; dy <= radius; ++dy)
    {
        for (int dx = -radius; dx <= radius; ++dx)
        {
            if (dx * dx + dy * dy <= radiusSquared)
            {
                appendDisplayCell(RecordCurrent_x + dx, RecordCurrent_y + dy);
            }
        }
    }
    return cells;
}

// 确保新bin值在表格中有一行颜色和计数配置。
void MapTest::ensureMapTypeRow(const QString& value)
{
    if (value.isEmpty() ||
        value == QStringLiteral(" ") ||
        value == ProcessedCellText ||
        value == m_processedCellText)
    {
        return;
    }

    for (int row = 0; row < ui->tableWidget->rowCount(); ++row)
    {
        QTableWidgetItem* dataItem = ui->tableWidget->item(row, 1);
        if (dataItem != nullptr && dataItem->text() == value)
        {
            return;
        }
    }

    int insertRow = qMax(0, ui->tableWidget->rowCount() - 1);
    ui->tableWidget->insertRow(insertRow);
    setTableCheckBox(ui->tableWidget, insertRow, 0, false);
    ui->tableWidget->setItem(insertRow, 1, new QTableWidgetItem(value));
    ui->tableWidget->setItem(insertRow, 2, new QTableWidgetItem(QStringLiteral("0")));
    ui->tableWidget->setItem(insertRow, 3, new QTableWidgetItem(QStringLiteral("0")));
    QColor color(rand() % 256, rand() % 256, rand() % 256);
    ui->tableWidget->setItem(insertRow, 4, new QTableWidgetItem(color.name()));
    ui->tableWidget->item(insertRow, 4)->setBackground(color);
}

// 按当前地图内容重新统计表格中的总数和剩余数量。
void MapTest::updateTableCountsFromMapData()
{
    std::unordered_map<std::string, int> counts;
    for (int row = 0; row < map_rows; ++row)
    {
        const auto rowIter = m_mapData.constFind(row);
        if (rowIter == m_mapData.constEnd())
        {
            continue;
        }

        for (int col = 0; col < map_columns; ++col)
        {
            const auto cellIter = rowIter.value().constFind(col);
            counts[cellIter == rowIter.value().constEnd() ? BlankCellValue : cellIter.value()]++;
        }
    }

    std::unordered_map<std::string, int> processedByValue;
    for (auto it = m_processedOriginalValues.constBegin(); it != m_processedOriginalValues.constEnd(); ++it)
    {
        processedByValue[it.value()]++;
    }

    for (int row = 0; row < ui->tableWidget->rowCount(); ++row)
    {
        QTableWidgetItem* dataItem = ui->tableWidget->item(row, 1);
        if (dataItem == nullptr)
        {
            continue;
        }

        QString value = dataItem->text();
        const bool processedRow = isProcessedTableRow(row);
        std::string stdValue = value.toStdString();
        int remaining = processedRow ? 0 : counts[stdValue];
        int count = processedRow ? m_processedSourceCells.size() : remaining + processedByValue[stdValue];

        QTableWidgetItem* totalItem = ui->tableWidget->item(row, 2);
        if (totalItem == nullptr)
        {
            totalItem = new QTableWidgetItem();
            ui->tableWidget->setItem(row, 2, totalItem);
        }
        totalItem->setText(QString::number(count));

        QTableWidgetItem* remainingItem = ui->tableWidget->item(row, 3);
        if (remainingItem == nullptr)
        {
            remainingItem = new QTableWidgetItem();
            ui->tableWidget->setItem(row, 3, remainingItem);
        }
        remainingItem->setText(QString::number(remaining));
    }
}

// 将当前编辑选区改成新bin，传空白时相当于裁剪为空白区域。
void MapTest::applyEditToSelection(const QString& newValue)
{
    if (m_mapData.isEmpty())
    {
        return;
    }

    QString value = newValue.isEmpty() ? QStringLiteral(" ") : newValue.left(1);
    if (value == m_processedCellText)
    {
        QMessageBox::warning(this,
                             QStringLiteral("Map编辑"),
                             QStringLiteral("新bin不能和已加工字符“%1”相同。").arg(m_processedCellText));
        return;
    }

    QVector<QPoint> cells = editSelectionSourceCells();
    if (cells.isEmpty())
    {
        return;
    }

    ensureMapTypeRow(value);
    std::string stdValue = value.toStdString();
    for (const QPoint& sourceCell : cells)
    {
        qint64 key = sourceCellKey(sourceCell.x(), sourceCell.y());
        m_processedSourceCells.remove(key);
        m_processedOriginalValues.remove(key);
        m_mapData[sourceCell.y()][sourceCell.x()] = stdValue;
    }

    rebuildRotatedMap();
    updateTableCountsFromMapData();
    m_pendingPathDisplayCells.clear();
    m_assistPathSourceCells.clear();
    m_hasFoundStartPoint = false;
    clearFindStartRoute();
    invalidateMapColorImage();
    m_hasEditRectStart = false;
    m_hasEditRectEnd = false;
    updateEditSelectionUi();
    refreshMapViews();
}

// 在大图上绘制当前框选或圆选范围。
void MapTest::drawMainMapEditSelection(QPainter& painter, const QRectF& view, double cellWidth, double cellHeight)
{
    if (m_editSelectionMode == MapEditRect && m_hasEditRectStart && m_hasEditRectEnd)
    {
        int left = qMin(m_editRectStartDisplay.x(), m_editRectEndDisplay.x());
        int right = qMax(m_editRectStartDisplay.x(), m_editRectEndDisplay.x());
        int top = qMin(m_editRectStartDisplay.y(), m_editRectEndDisplay.y());
        int bottom = qMax(m_editRectStartDisplay.y(), m_editRectEndDisplay.y());
        QRectF rect((left - view.left()) * cellWidth,
                    (top - view.top()) * cellHeight,
                    (right - left + 1) * cellWidth,
                    (bottom - top + 1) * cellHeight);
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setPen(QPen(QColor(255, 170, 0), 2, Qt::DashLine));
        painter.setBrush(QColor(255, 170, 0, 45));
        painter.drawRect(rect.adjusted(1, 1, -1, -1));
        painter.restore();
        return;
    }

    if (m_editSelectionMode == MapEditCircle)
    {
        int radius = qMax(1, ui->ring_count_spinBox->value());
        QPointF center((RecordCurrent_x + 0.5 - view.left()) * cellWidth,
                       (RecordCurrent_y + 0.5 - view.top()) * cellHeight);
        QRectF ellipse(center.x() - radius * cellWidth,
                       center.y() - radius * cellHeight,
                       radius * 2 * cellWidth,
                       radius * 2 * cellHeight);
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(QColor(255, 170, 0), 2, Qt::DashLine));
        painter.setBrush(QColor(255, 170, 0, 35));
        painter.drawEllipse(ellipse);
        painter.restore();
    }
}

// 在小图上绘制当前框选或圆选范围，并裁切到小图区域内。
void MapTest::drawSmallMapEditSelection(QPainter& painter, int startX, int startY, double cellWidth, double cellHeight)
{
    if (m_editSelectionMode == MapEditRect && m_hasEditRectStart && m_hasEditRectEnd)
    {
        int left = qMin(m_editRectStartDisplay.x(), m_editRectEndDisplay.x());
        int right = qMax(m_editRectStartDisplay.x(), m_editRectEndDisplay.x());
        int top = qMin(m_editRectStartDisplay.y(), m_editRectEndDisplay.y());
        int bottom = qMax(m_editRectStartDisplay.y(), m_editRectEndDisplay.y());
        QRectF rect((left - startX) * cellWidth,
                    (top - startY) * cellHeight,
                    (right - left + 1) * cellWidth,
                    (bottom - top + 1) * cellHeight);
        painter.save();
        painter.setClipRect(QRectF(0, 0, ui->small_label->width(), ui->small_label->height()));
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setPen(QPen(QColor(255, 170, 0), 2, Qt::DashLine));
        painter.setBrush(QColor(255, 170, 0, 45));
        painter.drawRect(rect.adjusted(1, 1, -1, -1));
        painter.restore();
        return;
    }

    if (m_editSelectionMode == MapEditCircle)
    {
        int radius = qMax(1, ui->ring_count_spinBox->value());
        QPointF center((RecordCurrent_x - startX + 0.5) * cellWidth,
                       (RecordCurrent_y - startY + 0.5) * cellHeight);
        QRectF ellipse(center.x() - radius * cellWidth,
                       center.y() - radius * cellHeight,
                       radius * 2 * cellWidth,
                       radius * 2 * cellHeight);
        painter.save();
        painter.setClipRect(QRectF(0, 0, ui->small_label->width(), ui->small_label->height()));
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(QColor(255, 170, 0), 2, Qt::DashLine));
        painter.setBrush(QColor(255, 170, 0, 35));
        painter.drawEllipse(ellipse);
        painter.restore();
    }
}

// 将大图视图中心移动到指定地图格。
void MapTest::centerViewOnCell(int x, int y)
{
    m_viewCenterCell = QPointF(clampToRange(x, 0, displayColumns() - 1) + 0.5,
                               clampToRange(y, 0, displayRows() - 1) + 0.5);
}

// 以鼠标锚点为中心缩放大图，尽量保持锚点位置不漂移。
void MapTest::zoomMainMap(const QPoint& anchor, double factor)
{
    if (m_mapData.isEmpty())
    {
        return;
    }

    QRectF oldView = currentViewRect();
    double anchorX = oldView.left() + anchor.x() * oldView.width() / qMax(1, ui->main_windows_label->width());
    double anchorY = oldView.top() + anchor.y() * oldView.height() / qMax(1, ui->main_windows_label->height());
    double oldScale = m_scaleFactor;
    m_scaleFactor = clampToRange(m_scaleFactor * factor, m_minScaleFactor, m_maxScaleFactor);
    if (qFuzzyCompare(oldScale, m_scaleFactor))
    {
        return;
    }

    double newViewCols = qMax(1.0, displayColumns() / m_scaleFactor);
    double newViewRows = qMax(1.0, displayRows() / m_scaleFactor);
    double anchorRatioX = anchor.x() / (double)qMax(1, ui->main_windows_label->width());
    double anchorRatioY = anchor.y() / (double)qMax(1, ui->main_windows_label->height());
    double left = anchorX - anchorRatioX * newViewCols;
    double top = anchorY - anchorRatioY * newViewRows;

    m_viewCenterCell = QPointF(left + newViewCols / 2.0, top + newViewRows / 2.0);
    refreshMapViews();
}

// 按鼠标拖拽的像素偏移平移大图视图。
void MapTest::panMainMap(const QPoint& pixelDelta)
{
    if (m_mapData.isEmpty())
    {
        return;
    }

    QRectF view = currentViewRect();
    double dx = -pixelDelta.x() * view.width() / qMax(1, ui->main_windows_label->width());
    double dy = -pixelDelta.y() * view.height() / qMax(1, ui->main_windows_label->height());
    m_viewCenterCell += QPointF(dx, dy);
    refreshMapViews();
}

// 重绘大图：只绘制当前可见区域、选中框、编辑选区和点位标记。
void MapTest::renderMainMap()
{
    if (m_mapData.isEmpty())
    {
        ui->main_windows_label->clear();
        ui->main_windows_label->update();
        ui->small_label->clear();
        ui->small_label->update();
        return;
    }

    QElapsedTimer mstimer;
    mstimer.start();

    QPixmap pixmap(ui->main_windows_label->size());
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, false);

    QRectF view = currentViewRect();
    const MapData& mapData = currentMapData();
    std::unordered_map<std::string, QColor> colorMap = buildColorMap();
    const int columns = displayColumns();
    const int rows = displayRows();

    int startCol = clampToRange((int)floor(view.left()), 0, columns - 1);
    int endCol = clampToRange((int)ceil(view.right()), 0, columns);
    int startRow = clampToRange((int)floor(view.top()), 0, rows - 1);
    int endRow = clampToRange((int)ceil(view.bottom()), 0, rows);
    double cellWidth = ui->main_windows_label->width() / view.width();
    double cellHeight = ui->main_windows_label->height() / view.height();
    const int pixelWidth = ui->main_windows_label->width();
    const int pixelHeight = ui->main_windows_label->height();
    const bool drawCellText = cellWidth >= 10 && cellHeight >= 10;
    const std::string blankCell(" ");

    auto cellColor = [this, &colorMap](const std::string& value, int col, int row) {
        if (isFindStartPathDisplayCell(col, row))
        {
            return QColor(255, 178, 70);
        }
        if (isAssistPathDisplayCell(col, row))
        {
            return QColor(165, 165, 165);
        }

        auto iter = colorMap.find(value);
        return (iter != colorMap.end()) ? iter->second : QColor(Qt::white);
    };

    if (!drawCellText)
    {
        if (m_mapColorImageDirty ||
            m_mapColorImage.width() != columns ||
            m_mapColorImage.height() != rows)
        {
            rebuildMapColorImage(colorMap);
        }

        if (!m_mapColorImage.isNull())
        {
            painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
            painter.drawImage(QRectF(0, 0, pixelWidth, pixelHeight), m_mapColorImage, view);
        }
    }
    else
    {
        QFont cellFont = painter.font();
        cellFont.setPointSizeF(qBound(6.0, qMin(cellWidth, cellHeight) * 0.55, 12.0));
        painter.setFont(cellFont);

        for (int row = startRow; row < endRow; ++row)
        {
            const auto rowIter = mapData.constFind(row);
            if (rowIter == mapData.constEnd())
            {
                continue;
            }

            for (int col = startCol; col < endCol; ++col)
            {
                const auto cellIter = rowIter.value().constFind(col);
                const std::string& ASCII = (cellIter == rowIter.value().constEnd()) ? blankCell : cellIter.value();
                QColor color = cellColor(ASCII, col, row);
                QRect rect(qFloor((col - view.left()) * cellWidth),
                           qFloor((row - view.top()) * cellHeight),
                           qCeil(cellWidth) + 1,
                           qCeil(cellHeight) + 1);
                painter.fillRect(rect, color);

                if (ASCII != blankCell)
                {
                    painter.setPen(Qt::black);
                    painter.drawText(rect, Qt::AlignCenter, QString::fromStdString(ASCII));
                }
            }
        }
    }

    if (cellWidth >= 6 && cellHeight >= 6)
    {
        painter.setPen(QPen(QColor(40, 40, 40), 1));
        for (int col = startCol; col <= endCol; ++col)
        {
            double x = (col - view.left()) * cellWidth;
            painter.drawLine(QPointF(x, 0), QPointF(x, pixelHeight));
        }
        for (int row = startRow; row <= endRow; ++row)
        {
            double y = (row - view.top()) * cellHeight;
            painter.drawLine(QPointF(0, y), QPointF(pixelWidth, y));
        }
    }

    const int smallMapCellCount = 15;
    QRectF smallMapRect((RecordCurrent_x - smallMapCellCount / 2 - view.left()) * cellWidth,
                        (RecordCurrent_y - smallMapCellCount / 2 - view.top()) * cellHeight,
                        smallMapCellCount * cellWidth,
                        smallMapCellCount * cellHeight);
    painter.setPen(QPen(Qt::red, 2));
    painter.drawRect(smallMapRect.adjusted(1, 1, -1, -1));

    QRectF selectedRect((RecordCurrent_x - view.left()) * cellWidth,
                        (RecordCurrent_y - view.top()) * cellHeight,
                        cellWidth,
                        cellHeight);
    painter.setPen(QPen(Qt::red, 3));
    painter.drawRect(selectedRect.adjusted(1, 1, -1, -1));
    drawMainMapEditSelection(painter, view, cellWidth, cellHeight);
    drawMainMapMarkers(painter, view, cellWidth, cellHeight);

    ui->main_windows_label->setPixmap(pixmap);
    ui->main_windows_label->update();
    ui->label->setText("运行时间: " + QString::number(mstimer.nsecsElapsed() / 1000000.0, 'f', 2) + "ms");
    renderSmallMap(colorMap);
}

// 重绘小图，以当前选中格为中心显示15x15局部区域。
void MapTest::renderSmallMap(const std::unordered_map<std::string, QColor>& colorMap)
{
    if (m_mapData.isEmpty())
    {
        ui->small_label->clear();
        ui->small_label->update();
        return;
    }

    DrawSmallMap(ui->small_label, RecordCurrent_x, RecordCurrent_y, colorMap);
    ui->small_label->update();
}

// 统一刷新坐标信息、刻度尺、大图和小图。
void MapTest::refreshMapViews()
{
    updateSelectionUi();
    updateRulers();
    renderMainMap();
}

// 刷新大图下方文件名、行号、列号和角度信息。
void MapTest::updateSelectionUi()
{
    QString fileName = filTexyename.isEmpty()
            ? QStringLiteral("--")
            : QFileInfo(filTexyename).fileName();

    QString rowText = m_mapData.isEmpty()
            ? QStringLiteral("--")
            : QString::number(RecordCurrent_y + 1);
    QString columnText = m_mapData.isEmpty()
            ? QStringLiteral("--")
            : QString::number(RecordCurrent_x + 1);

    ui->map_info_label->setText(QStringLiteral("文件名：%1\n行号：%2  列号：%3  角度：%4度")
                                .arg(fileName)
                                .arg(rowText)
                                .arg(columnText)
                                .arg(rodegrees));

    updatePointAlignmentUi();
    updateEditSelectionUi();
}

// 刷新参考点、验证点和起点坐标显示。
void MapTest::updatePointAlignmentUi()
{
    if (m_mapData.isEmpty())
    {
        ui->point_alignment_info_label->setText(QStringLiteral("参考点：--, --\n验证点1：--, --\n起点：--, --"));
        return;
    }

    auto coordinateText = [this](const QPoint& sourceCell) {
        QPoint displayCell = sourceToDisplayCell(sourceCell.x(), sourceCell.y());
        return QStringLiteral("%1, %2").arg(displayCell.x() + 1).arg(displayCell.y() + 1);
    };

    QStringList lines;
    lines << QStringLiteral("参考点：%1").arg(m_hasReferencePoint
                                             ? coordinateText(m_referencePointSource)
                                             : QStringLiteral("--, --"));

    if (m_verifyPointsSource.isEmpty())
    {
        lines << QStringLiteral("验证点1：--, --");
    }
    else
    {
        for (int i = 0; i < m_verifyPointsSource.size(); ++i)
        {
            lines << QStringLiteral("验证点%1：%2").arg(i + 1).arg(coordinateText(m_verifyPointsSource.at(i)));
        }
    }

    lines << QStringLiteral("起点：%1").arg(m_hasStartPoint
                                            ? coordinateText(m_startPointSource)
                                            : QStringLiteral("--, --"));

    ui->point_alignment_info_label->setText(lines.join(QStringLiteral("\n")));
}

// 刷新方向按钮的文字、图标和提示。
void MapTest::updateDirectionButton()
{
    ui->direction_pushButton->setIcon(directionIcon(m_selectedDirection));
    ui->direction_pushButton->setIconSize(QSize(52, 34));
    ui->direction_pushButton->setText(QStringLiteral("行进方向:%1").arg(directionText(m_selectedDirection)));
    ui->direction_pushButton->setToolTip(QStringLiteral("点击选择行进方向"));
}

// 绑定右侧Map功能区的按钮、单选框和手动扫码动作。
void MapTest::setupMapFunctionPanel()
{
    connect(ui->map_function_comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        QWidget* targetPage = ui->setting_page;
        if (index == 1)
        {
            targetPage = ui->point_page;
        }
        else if (index == 2)
        {
            targetPage = ui->path_page;
        }
        else if (index == 3)
        {
            targetPage = ui->param_page;
        }
        ui->map_function_stack->setCurrentWidget(targetPage);

        // 离开Map编辑页时恢复点选，避免圆选/框选的图形一直残留在地图上。
        const bool leaveEditPage = targetPage != ui->param_page;
        if (leaveEditPage && m_editSelectionMode != MapEditPoint)
        {
            m_hasEditRectStart = false;
            m_hasEditRectEnd = false;
            if (ui->edit_point_radioButton->isChecked())
            {
                setEditSelectionMode(MapEditPoint);
            }
            else
            {
                ui->edit_point_radioButton->setChecked(true);
            }
        }
    });

    connect(ui->manual_scan_pushButton, &QPushButton::clicked, this, [this]() {
        QDialog dialog(this);
        dialog.setWindowTitle(QStringLiteral("手动扫码"));
        dialog.setModal(true);
        dialog.setMinimumWidth(460);

        QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);
        mainLayout->addWidget(new QLabel(QStringLiteral("扫码内容/文件名"), &dialog));

        QLineEdit* scanLineEdit = new QLineEdit(&dialog);
        scanLineEdit->setPlaceholderText(QStringLiteral("例如：22030290-4 或 22030290-4.txt"));
        mainLayout->addWidget(scanLineEdit);

        QRadioButton* directoryRadioButton = new QRadioButton(QStringLiteral("指定目录搜索"), &dialog);
        QRadioButton* fullDiskRadioButton = new QRadioButton(QStringLiteral("全盘搜索"), &dialog);
        directoryRadioButton->setChecked(true);
        mainLayout->addWidget(directoryRadioButton);

        QHBoxLayout* directoryLayout = new QHBoxLayout();
        QLineEdit* directoryLineEdit = new QLineEdit(&dialog);
        directoryLineEdit->setReadOnly(true);
        QString defaultDirectory = !m_lastScanSearchPath.isEmpty()
                ? m_lastScanSearchPath
                : (!filTexyename.isEmpty() ? QFileInfo(filTexyename).absolutePath() : QDir::homePath());
        directoryLineEdit->setText(QDir::toNativeSeparators(defaultDirectory));
        QPushButton* chooseDirectoryButton = new QPushButton(QStringLiteral("选择目录"), &dialog);
        directoryLayout->addWidget(directoryLineEdit, 1);
        directoryLayout->addWidget(chooseDirectoryButton);
        mainLayout->addLayout(directoryLayout);
        mainLayout->addWidget(fullDiskRadioButton);

        connect(chooseDirectoryButton, &QPushButton::clicked, &dialog, [this, directoryLineEdit, directoryRadioButton]() {
            QString startDirectory = QDir::fromNativeSeparators(directoryLineEdit->text());
            QString directory = QFileDialog::getExistingDirectory(this,
                                                                  QStringLiteral("选择搜索目录"),
                                                                  startDirectory.isEmpty() ? QDir::homePath() : startDirectory);
            if (!directory.isEmpty())
            {
                directoryLineEdit->setText(QDir::toNativeSeparators(directory));
                directoryRadioButton->setChecked(true);
            }
        });

        QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确认"));
        buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
        connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        mainLayout->addWidget(buttonBox);

        if (dialog.exec() != QDialog::Accepted || scanLineEdit->text().trimmed().isEmpty())
        {
            return;
        }

        QString scanText = scanLineEdit->text().trimmed();
        QStringList searchRoots;
        if (fullDiskRadioButton->isChecked())
        {
            const QFileInfoList drives = QDir::drives();
            for (const QFileInfo& drive : drives)
            {
                searchRoots << drive.absoluteFilePath();
            }
        }
        else
        {
            QString directory = QDir::fromNativeSeparators(directoryLineEdit->text());
            if (directory.isEmpty() || !QDir(directory).exists())
            {
                QMessageBox::warning(this,
                                     QStringLiteral("手动扫码"),
                                     QStringLiteral("请先选择有效的搜索目录。"));
                return;
            }
            m_lastScanSearchPath = directory;
            searchRoots << directory;
        }

        QApplication::setOverrideCursor(Qt::WaitCursor);
        QString filePath = searchMapFileByScanText(scanText, searchRoots);
        QApplication::restoreOverrideCursor();
        if (filePath.isEmpty())
        {
            QMessageBox::warning(this,
                                 QStringLiteral("手动扫码"),
                                 QStringLiteral("没有找到对应地图文件：%1").arg(scanText.trimmed()));
            return;
        }

        int selectedLoadMode = ui->map_load_mode_comboBox->currentIndex();
        loadMapFile(filePath, selectedLoadMode);
    });

    connect(ui->reset_view_pushButton, &QPushButton::clicked, this, [this]() {
        resetMapScale();
    });
    connect(ui->first_target_pushButton, &QPushButton::clicked, this, [this]() {
        std::unordered_map<std::string, int> checkedRows = checkedTargetRows();
        std::vector<std::string> targetChars;
        targetChars.reserve(checkedRows.size());
        for (const auto& checkedRow : checkedRows)
        {
            targetChars.push_back(checkedRow.first);
        }
        std::pair<int, int> firstTarget = findFirstTargetPosition(currentMapData(), targetChars, m_selectedDirection);
        if (firstTarget.first >= 0 && firstTarget.second >= 0)
        {
            setSelectedCell(firstTarget.second, firstTarget.first, true, false);
            setStartPointFromSelection();
        }
    });

    connect(ui->set_start_point_pushButton, &QPushButton::clicked, this, &MapTest::setStartPointFromSelection);
    connect(ui->set_reference_point_pushButton, &QPushButton::clicked, this, &MapTest::setReferencePointFromSelection);
    connect(ui->ref_point_pushButton, &QPushButton::clicked, this, &MapTest::addVerifyPointFromSelection);
    connect(ui->clear_reference_points_pushButton, &QPushButton::clicked, this, &MapTest::clearVerifyPoints);
    connect(ui->find_start_pushButton, &QPushButton::clicked, this, [this]() {
        if (m_isFindingStartPoint)
        {
            stopFindStartPoint(QStringLiteral("寻起点已停止"));
            return;
        }
        findStartPointRoute();
    });

    connect(ui->edit_point_radioButton, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked)
        {
            setEditSelectionMode(MapEditPoint);
        }
    });
    connect(ui->edit_rect_radioButton, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked)
        {
            setEditSelectionMode(MapEditRect);
        }
    });
    connect(ui->edit_circle_radioButton, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked)
        {
            setEditSelectionMode(MapEditCircle);
        }
    });
    connect(ui->ring_count_spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        if (m_editSelectionMode == MapEditCircle)
        {
            refreshMapViews();
        }
    });
    connect(ui->edit_current_point_pushButton, &QPushButton::clicked, this, [this]() {
        centerViewOnCell(RecordCurrent_x, RecordCurrent_y);
        refreshMapViews();
    });
    connect(ui->edit_left_top_pushButton, &QPushButton::clicked, this, &MapTest::setEditRectStartFromSelection);
    connect(ui->edit_right_bottom_pushButton, &QPushButton::clicked, this, &MapTest::setEditRectEndFromSelection);
    connect(ui->change_bin_pushButton, &QPushButton::clicked, this, [this]() {
        applyEditToSelection(ui->new_bin_lineEdit->text());
    });
    connect(ui->crop_select_pushButton, &QPushButton::clicked, this, [this]() {
        applyEditToSelection(QStringLiteral(" "));
    });

    ui->map_function_comboBox->setCurrentIndex(0);
    ui->map_function_stack->setCurrentIndex(ui->map_function_comboBox->currentIndex());
}

// 启动加工时锁定地图加载、方向和编辑控件，防止运行中修改状态。
void MapTest::setMapFunctionPanelRunning(bool running)
{
    ui->map_function_comboBox->setEnabled(!running);
    ui->map_load_mode_comboBox->setEnabled(!running);
    ui->read_pushButton->setEnabled(!running);
    ui->manual_scan_pushButton->setEnabled(!running);
    ui->reset_view_pushButton->setEnabled(!running);
    ui->rotate->setEnabled(!running);
    ui->direction_pushButton->setEnabled(!running);
    const bool pathPlanningRunning = running && !m_isFindingStartPoint;
    ui->start_pushButton->setEnabled(!running || pathPlanningRunning);
    ui->start_pushButton->setText(pathPlanningRunning
                                  ? QStringLiteral("停止")
                                  : QStringLiteral("开始规划"));
    ui->first_target_pushButton->setEnabled(!running);
    ui->set_start_point_pushButton->setEnabled(!running);
    ui->set_reference_point_pushButton->setEnabled(!running);
    ui->ref_point_pushButton->setEnabled(!running);
    ui->clear_reference_points_pushButton->setEnabled(!running);
    ui->find_start_pushButton->setEnabled(!running || m_isFindingStartPoint);
    ui->find_start_pushButton->setText(m_isFindingStartPoint ? QStringLiteral("停止") : QStringLiteral("寻起点"));
    ui->edit_point_radioButton->setEnabled(!running);
    ui->edit_rect_radioButton->setEnabled(!running);
    ui->edit_circle_radioButton->setEnabled(!running);
    ui->edit_current_point_pushButton->setEnabled(!running);
    ui->edit_left_top_pushButton->setEnabled(!running);
    ui->edit_right_bottom_pushButton->setEnabled(!running);
    ui->change_bin_pushButton->setEnabled(!running);
    ui->crop_select_pushButton->setEnabled(!running);
    ui->new_bin_lineEdit->setEnabled(!running);
    ui->ring_count_spinBox->setEnabled(!running);

    for (int row = 0; row < ui->tableWidget->rowCount(); ++row)
    {
        QWidget* widget = ui->tableWidget->cellWidget(row, 0);
        if (widget == nullptr)
        {
            continue;
        }

        const QList<QCheckBox*> boxes = widget->findChildren<QCheckBox*>();
        for (QCheckBox* box : boxes)
        {
            box->setEnabled(!running);
        }
    }
}

// 弹出方向选择窗口，用户确认后更新蛇形行进方向。
void MapTest::on_direction_pushButton_clicked()
{
    if (m_pTimer->isActive())
    {
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("选择行进方向"));
    dialog.setModal(true);
    dialog.setMinimumWidth(420);

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);

    QLabel* titleLabel = new QLabel(QStringLiteral("选择行进方向"), &dialog);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(12);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    QGroupBox* directionGroup = new QGroupBox(QStringLiteral("行进方向"), &dialog);
    QGridLayout* gridLayout = new QGridLayout(directionGroup);
    gridLayout->setHorizontalSpacing(16);
    gridLayout->setVerticalSpacing(12);

    QButtonGroup buttonGroup(&dialog);
    buttonGroup.setExclusive(true);

    const Direction directions[] = {
        LeftToRightTopToBottom,
        LeftToRightBottomToTop,
        RightToLeftTopToBottom,
        RightToLeftBottomToTop,
        TopToBottomLeftToRight,
        TopToBottomRightToLeft,
        BottomToTopLeftToRight,
        BottomToTopRightToLeft
    };

    for (int i = 0; i < 8; ++i)
    {
        Direction direction = directions[i];
        QWidget* tile = new QWidget(directionGroup);
        QVBoxLayout* tileLayout = new QVBoxLayout(tile);
        tileLayout->setContentsMargins(4, 4, 4, 4);
        tileLayout->setSpacing(2);

        QRadioButton* radio = new QRadioButton(tile);
        radio->setIcon(directionIcon(direction));
        radio->setIconSize(QSize(74, 48));
        radio->setToolTip(directionText(direction));
        radio->setChecked(direction == m_selectedDirection);
        buttonGroup.addButton(radio, static_cast<int>(direction));

        QLabel* textLabel = new QLabel(directionText(direction), tile);
        textLabel->setAlignment(Qt::AlignCenter);

        tileLayout->addWidget(radio, 0, Qt::AlignCenter);
        tileLayout->addWidget(textLabel);
        gridLayout->addWidget(tile, i / 4, i % 4);
    }

    mainLayout->addWidget(directionGroup);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(&dialog);
    QPushButton* okButton = buttonBox->addButton(QStringLiteral("确定"), QDialogButtonBox::AcceptRole);
    QPushButton* cancelButton = buttonBox->addButton(QStringLiteral("取消"), QDialogButtonBox::RejectRole);
    okButton->setIcon(dialog.style()->standardIcon(QStyle::SP_DialogApplyButton));
    cancelButton->setIcon(dialog.style()->standardIcon(QStyle::SP_DialogCancelButton));
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    if (dialog.exec() == QDialog::Accepted)
    {
        int id = buttonGroup.checkedId();
        if (id >= 0)
        {
            setMapDirection(static_cast<Direction>(id));
        }
    }
}

// 刷新四个刻度尺：大图显示全地图坐标位置，小图显示当前15x15局部范围。
void MapTest::updateRulers()
{
    const int columns = displayColumns();
    const int rows = displayRows();
    if (columns <= 0 || rows <= 0)
    {
        BigMapHorizontalslider->setRulerSliderRange(0, 1);
        BigMapHorizontalslider->setRulerSliderValue(0);
        BigMapVerticalslider->setRulerSliderRange(0, 1);
        BigMapVerticalslider->setRulerSliderValue(0);
        SmallMapHorizontalslider->setRulerSliderRange(0, 1);
        SmallMapHorizontalslider->setRulerSliderValue(0);
        SmallMapVerticalslider->setRulerSliderRange(0, 1);
        SmallMapVerticalslider->setRulerSliderValue(0);
        return;
    }

    BigMapHorizontalslider->setRulerSliderRange(1, columns);
    BigMapHorizontalslider->setRulerSliderValue(RecordCurrent_x + 1);
    BigMapVerticalslider->setRulerSliderRange(1, rows);
    BigMapVerticalslider->setRulerSliderValue(RecordCurrent_y + 1);

    int smallStartX = qMax(1, RecordCurrent_x - 7 + 1);
    int smallEndX = qMin(columns, RecordCurrent_x + 8 + 1);
    int smallStartY = qMax(1, RecordCurrent_y - 7 + 1);
    int smallEndY = qMin(rows, RecordCurrent_y + 8 + 1);
    SmallMapHorizontalslider->setRulerSliderRange(smallStartX, smallEndX);
    SmallMapHorizontalslider->setRulerSliderValue(RecordCurrent_x + 1);
    SmallMapVerticalslider->setRulerSliderRange(smallStartY, smallEndY);
    SmallMapVerticalslider->setRulerSliderValue(RecordCurrent_y + 1);
}

// 初始化表格的紧凑显示，避免颜色列被挤出后出现横向滚动条。
void MapTest::setupMapTableView()
{
    ui->tableWidget->setEditTriggers(QTableWidget::NoEditTriggers);
    ui->tableWidget->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->tableWidget->setWordWrap(false);
    ui->tableWidget->setAlternatingRowColors(false);

    QFont tableFont = ui->tableWidget->font();
    tableFont.setPointSize(10);
    ui->tableWidget->setFont(tableFont);
    ui->tableWidget->horizontalHeader()->setFont(tableFont);
    ui->tableWidget->verticalHeader()->setFont(tableFont);
    ui->tableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(false);
    ui->tableWidget->verticalHeader()->setDefaultSectionSize(24);
    ui->tableWidget->verticalHeader()->setMinimumSectionSize(20);
    ui->tableWidget->verticalHeader()->setFixedWidth(28);
    ui->tableWidget->setShowGrid(true);
    updateTableColumnWidths();
}

// 从UI动态属性columnWidths读取初始列宽；之后允许用户拖动表头分隔线调整。
void MapTest::updateTableColumnWidths()
{
    if (ui->tableWidget->columnCount() < 5)
    {
        return;
    }

    QVector<int> widths = {76, 54, 86, 86, 74};
    QString widthConfig = ui->tableWidget->property("columnWidths").toString();
    widthConfig.replace(QStringLiteral("，"), QStringLiteral(","));
    const QStringList parts = widthConfig.split(QStringLiteral(","), Qt::SkipEmptyParts);
    for (int i = 0; i < parts.size() && i < widths.size(); ++i)
    {
        bool ok = false;
        const int width = parts.at(i).trimmed().toInt(&ok);
        if (ok && width > 0)
        {
            widths[i] = width;
        }
    }

    for (int i = 0; i < widths.size(); ++i)
    {
        ui->tableWidget->setColumnWidth(i, widths.at(i));
    }
}

// 表格和小图区域允许在UI里直接拖动位置和尺寸，运行时只做边界保护。
void MapTest::captureSmallMapChildLayoutBase()
{
    m_baseTableWidgetRect = ui->tableWidget->geometry();
    m_baseSmallLabelRect = ui->small_label->geometry();
    m_hasSmallMapChildLayoutBase = true;
}

// 根据三个UI模块的几何信息同步内部控件，避免控件改尺寸后还要改代码。
void MapTest::syncMapWidgetGeometry()
{
    if (m_isSyncingMapWidgetGeometry)
    {
        return;
    }
    if (!BigMapHorizontalslider || !BigMapVerticalslider ||
        !SmallMapHorizontalslider || !SmallMapVerticalslider)
    {
        return;
    }

    m_isSyncingMapWidgetGeometry = true;

    const QRect bigModuleRect = ui->big_map_module->geometry();
    const QRect smallModuleRect = ui->small_map_module->geometry();

    const int mapInfoHeight = qMax(1, ui->map_info_label->height());
    const int mainWidth = qMax(1, bigModuleRect.width() - RulerThickness);
    const int mainHeight = qMax(1, bigModuleRect.height() - RulerThickness - mapInfoHeight);
    QRect mainRect(0, 0, mainWidth, mainHeight);
    ui->main_windows_label->setGeometry(mainRect);
    ui->map_info_label->setGeometry(0,
                                    mainHeight + RulerThickness,
                                    mainWidth,
                                    mapInfoHeight);

    BigMapHorizontalslider->setFixedSize(mainWidth, RulerThickness);
    BigMapHorizontalslider->move(bigModuleRect.left(), bigModuleRect.top() + mainHeight);
    BigMapVerticalslider->setFixedSize(RulerThickness, mainHeight);
    BigMapVerticalslider->move(bigModuleRect.left() + mainWidth, bigModuleRect.top());

    const int statusHeight = qMax(1, ui->label->height());
    const int smallContentWidth = qMax(1, smallModuleRect.width() - RulerThickness);
    if (!m_hasSmallMapChildLayoutBase)
    {
        captureSmallMapChildLayoutBase();
    }
    const int availableSmallHeight = qMax(1, smallModuleRect.height()
                                             - RulerThickness
                                             - statusHeight
                                             - ModuleVerticalGap
                                             - StatusVerticalGap);
    auto boundedChildRect = [availableSmallHeight](const QRect& baseRect, int availableWidth) {
        availableWidth = qMax(1, availableWidth);
        const int left = qBound(0, baseRect.left(), qMax(0, availableWidth - 1));
        const int top = qBound(0, baseRect.top(), qMax(0, availableSmallHeight - 1));
        const int width = qMin(qMax(1, baseRect.width()), qMax(1, availableWidth - left));
        const int height = qMin(qMax(1, baseRect.height()), qMax(1, availableSmallHeight - top));
        return QRect(left, top, width, height);
    };

    QRect tableRect = boundedChildRect(m_baseTableWidgetRect, smallContentWidth);
    QRect smallRect = boundedChildRect(m_baseSmallLabelRect, smallContentWidth);
    ui->tableWidget->setGeometry(tableRect);
    ui->small_label->setGeometry(smallRect);
    ui->label->setGeometry(smallRect.left(),
                           smallRect.top() + smallRect.height() + RulerThickness + StatusVerticalGap,
                           smallRect.width(),
                           statusHeight);

    SmallMapHorizontalslider->setFixedSize(smallRect.width(), RulerThickness);
    SmallMapHorizontalslider->move(smallModuleRect.left() + smallRect.left(), smallModuleRect.top() + smallRect.top() + smallRect.height());
    SmallMapVerticalslider->setFixedSize(RulerThickness, smallRect.height());
    SmallMapVerticalslider->move(smallModuleRect.left() + smallRect.left() + smallRect.width(), smallModuleRect.top() + smallRect.top());

    ui->big_map_module->raise();
    ui->small_map_module->raise();
    ui->main_windows_label->raise();
    ui->map_info_label->raise();
    ui->tableWidget->raise();
    ui->small_label->raise();
    ui->label->raise();
    ui->map_function_panel->raise();
    BigMapHorizontalslider->raise();
    BigMapVerticalslider->raise();
    SmallMapHorizontalslider->raise();
    SmallMapVerticalslider->raise();
    updateRulers();
    m_isSyncingMapWidgetGeometry = false;
}

// 按当前旋转角度重建绘图用的旋转地图数据。
void MapTest::rebuildRotatedMap()
{
    rotate_m_mapData.clear();
    invalidateMapColorImage();

    if (rodegrees == 0)
    {
        return;
    }

    for (int i = 0; i < map_rows; i++)
    {
        for (int j = 0; j < map_columns; j++)
        {
            if (rodegrees == 90)
            {
                rotate_m_mapData[j][map_rows - 1 - i] = m_mapData[i][j];
            }
            else if (rodegrees == 180)
            {
                rotate_m_mapData[map_rows - 1 - i][map_columns - 1 - j] = m_mapData[i][j];
            }
            else if (rodegrees == 270)
            {
                rotate_m_mapData[map_columns - 1 - j][i] = m_mapData[i][j];
            }
        }
    }
}

//恢复原始Map：优先重新读取当前文件，回到加载后的初始状态。
void MapTest::resetMapScale()
{
    if (!filTexyename.isEmpty() && QFileInfo::exists(filTexyename))
    {
        m_mapData.clear();
        rotate_m_mapData.clear();
        m_pendingPathDisplayCells.clear();
        m_processedSourceCells.clear();
        m_processedOriginalValues.clear();
        m_assistPathSourceCells.clear();
        clearFindStartRoute();
        invalidateMapColorImage();
        m_hasStartPoint = false;
        m_hasReferencePoint = false;
        m_hasFoundStartPoint = false;
        m_isFindingStartPoint = false;
        m_verifyPointsSource.clear();
        m_hasEditRectStart = false;
        m_hasEditRectEnd = false;
        ui->tableWidget->setRowCount(0);
        readMapTextFile();
        ui->label->setText(QStringLiteral(" 已恢复原始Map"));
        return;
    }

    if (m_mapData.isEmpty())
    {
        return;
    }

    m_scaleFactor = 1.0;
    RecordCurrent_x = clampToRange(RecordCurrent_x, 0, displayColumns() - 1);
    RecordCurrent_y = clampToRange(RecordCurrent_y, 0, displayRows() - 1);
    centerViewOnCell(RecordCurrent_x, RecordCurrent_y);
    refreshMapViews();
}
//鼠标滚动
void MapTest::wheelEvent(QWheelEvent *event)
{
    if (!m_mapData.isEmpty())
    {
        zoomMainMap(ui->main_windows_label->mapFrom(this, event->pos()),
                    event->angleDelta().y() > 0 ? 1.2 : 1.0 / 1.2);
        event->accept();
        return;
    }
    QMainWindow::wheelEvent(event);
}
// 窗口或布局尺寸变化后，同步刻度尺和地图绘制尺寸。
void MapTest::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    syncMapWidgetGeometry();
    if (!m_mapData.isEmpty())
    {
        refreshMapViews();
    }
}

//确认
void MapTest::on_sure_pushButton_clicked()
{
      if (m_mapData.isEmpty())
      {
          return;
      }

      processCurrentCellIfChecked();
      refreshMapViews();

}
//旋转
void MapTest::on_rotate_clicked()
{
	if (m_mapData.isEmpty()) { return; }
    QPoint sourceCell = displayToSourceCell(RecordCurrent_x, RecordCurrent_y);
    m_pendingPathDisplayCells.clear();
    m_assistPathSourceCells.clear();
    m_hasFoundStartPoint = false;
    clearFindStartRoute();
    rodegrees = (rodegrees + 90) % 360;
    rebuildRotatedMap();

    QPoint nextDisplayCell = QPoint(RecordCurrent_x, RecordCurrent_y);
    if (rodegrees == 90)
    {
        nextDisplayCell = QPoint(map_rows - 1 - sourceCell.y(), sourceCell.x());
    }
    else if (rodegrees == 180)
    {
        nextDisplayCell = QPoint(map_columns - 1 - sourceCell.x(), map_rows - 1 - sourceCell.y());
    }
    else if (rodegrees == 270)
    {
        nextDisplayCell = QPoint(sourceCell.y(), map_columns - 1 - sourceCell.x());
    }
    else
    {
        nextDisplayCell = sourceCell;
    }

    setSelectedCell(nextDisplayCell.x(), nextDisplayCell.y(), true);

}

// 表格点击处理：已加工字符可编辑，颜色列可弹出颜色选择器。
void MapTest::on_tableWidget_cellClicked(int row, int column)
{
    if (m_pTimer->isActive())
    {
        return;
    }

    if (isProcessedTableRow(row) && column == 1)
    {
        QTableWidgetItem* item = ui->tableWidget->item(row, column);
        if (item == nullptr)
        {
            item = new QTableWidgetItem(m_processedCellText);
            ui->tableWidget->setItem(row, column, item);
        }

        QWidget* oldWidget = ui->tableWidget->cellWidget(row, column);
        if (oldWidget != nullptr)
        {
            ui->tableWidget->removeCellWidget(row, column);
            delete oldWidget;
        }

        QLineEdit* editor = new QLineEdit(item->text(), ui->tableWidget);
        QRegularExpressionValidator* validator = new QRegularExpressionValidator(QRegularExpression("[!-~]"), editor);
        editor->setValidator(validator);
        editor->setMaxLength(1);
        ui->tableWidget->setCellWidget(row, column, editor);
        editor->selectAll();
        editor->setFocus();

        connect(editor, &QLineEdit::editingFinished, this, [this, editor, row, column]() {
            QString text = editor->text();
            ui->tableWidget->removeCellWidget(row, column);
            editor->deleteLater();

            if (!updateProcessedCellText(text, row))
            {
                QTableWidgetItem* item = ui->tableWidget->item(row, column);
                if (item != nullptr)
                {
                    item->setText(m_processedCellText);
                }
            }
        });
        return;
    }

    if(column==4)
    {
        QTableWidgetItem* item = ui->tableWidget->item(row, column);
            if (item)
            {
                // 弹出颜色选择框
                QColor color = QColorDialog::getColor(Qt::white, this, "Select Color");
                if (color.isValid())
                {
                    // 设置单元格背景颜色
                    item->setData(Qt::BackgroundColorRole, color);

                    item->setText(color.name());
                    // 输出颜色名称
                    invalidateMapColorImage();
                    refreshMapViews();
                }
            }
    }



}

// 智能借助：终点仍由原路径规则决定；不加工/空白区域按高代价处理，只有绕路太远时才借助。
QVector<QPoint> buildAssistPathToTarget(const MapData& m_mapDat,
                                        const std::vector<std::string>& targetChars,
                                        Direction direction,
                                        int startRow,
                                        int startCol,
                                        int targetRow,
                                        int targetCol,
                                        int currentMinorDx,
                                        int currentMinorDy)
{
    QVector<QPoint> path;
    const int ROWS = m_mapDat.size();
    if (ROWS <= 0)
    {
        return path;
    }
    const int COLS = m_mapDat.constBegin().value().size();
    if (COLS <= 0)
    {
        return path;
    }

    startRow = qBound(0, startRow, ROWS - 1);
    startCol = qBound(0, startCol, COLS - 1);
    targetRow = qBound(0, targetRow, ROWS - 1);
    targetCol = qBound(0, targetCol, COLS - 1);
    if (startRow == targetRow && startCol == targetCol)
    {
        return path;
    }

    std::unordered_set<std::string> targetSet(targetChars.begin(), targetChars.end());
    MapDirectionVectors vectors = directionVectors(direction);
    DirectionVector currentMinor{currentMinorDx, currentMinorDy};
    if (!isSameAxis(currentMinor, vectors.minor))
    {
        currentMinor = vectors.minor;
    }

    QVector<DirectionVector> neighbors;
    auto addNeighbor = [&](const DirectionVector& vector) {
        if (vector.dx == 0 && vector.dy == 0)
        {
            return;
        }
        for (const DirectionVector& existing : neighbors)
        {
            if (existing.dx == vector.dx && existing.dy == vector.dy)
            {
                return;
            }
        }
        neighbors.append(vector);
    };
    addNeighbor(currentMinor);
    addNeighbor(vectors.major);
    addNeighbor(negateVector(currentMinor));
    addNeighbor(negateVector(vectors.major));

    const int normalCost = 1;
    const int assistCost = 12; // 不加工/空白区域近似一堵墙，绕路超过约十几格才借助。
    auto stepCost = [&](int row, int col) {
        if ((row == targetRow && col == targetCol) || (row == startRow && col == startCol))
        {
            return normalCost;
        }

        const std::string& value = mapCellValue(m_mapDat, row, col);
        if (targetSet.find(value) != targetSet.end())
        {
            return normalCost;
        }
        return assistCost;
    };

    struct QueueNode
    {
        int cost;
        int order;
        int row;
        int col;
        bool operator<(const QueueNode& other) const
        {
            if (cost != other.cost)
            {
                return cost > other.cost;
            }
            return order > other.order;
        }
    };

    QVector<QVector<int>> distance(ROWS, QVector<int>(COLS, INT_MAX));
    QVector<QVector<QPoint>> parent(ROWS, QVector<QPoint>(COLS, QPoint(-1, -1)));
    std::priority_queue<QueueNode> queue;
    QPoint start(startCol, startRow);
    QPoint target(targetCol, targetRow);
    bool found = false;
    int visitOrder = 0;

    distance[startRow][startCol] = 0;
    queue.push({0, visitOrder++, startRow, startCol});
    while (!queue.empty() && !found)
    {
        QueueNode current = queue.top();
        queue.pop();
        if (current.cost != distance[current.row][current.col])
        {
            continue;
        }

        for (const DirectionVector& neighbor : neighbors)
        {
            int nextCol = current.col + neighbor.dx;
            int nextRow = current.row + neighbor.dy;
            if (nextCol < 0 || nextCol >= COLS || nextRow < 0 || nextRow >= ROWS)
            {
                continue;
            }

            int nextCost = current.cost + stepCost(nextRow, nextCol);
            if (nextCost >= distance[nextRow][nextCol])
            {
                continue;
            }

            distance[nextRow][nextCol] = nextCost;
            parent[nextRow][nextCol] = QPoint(current.col, current.row);
            if (nextRow == targetRow && nextCol == targetCol)
            {
                found = true;
                break;
            }
            queue.push({nextCost, visitOrder++, nextRow, nextCol});
        }
    }

    if (!found)
    {
        return path;
    }

    QVector<QPoint> reversedPath;
    QPoint cursor = target;
    while (cursor != start && cursor.x() >= 0 && cursor.y() >= 0)
    {
        reversedPath.append(cursor);
        cursor = parent[cursor.y()][cursor.x()];
    }

    for (int i = reversedPath.size() - 1; i >= 0; --i)
    {
        path.append(reversedPath.at(i));
    }

    return path;
}

// 按原有蛇形规则查找下一个加工目标，只决定终点，绕路由借助路径处理。
std::pair<int, int> traverseSMatrix(const MapData& m_mapDat,
                                    const std::vector<std::string>& targetChars,
                                    Direction direction,
                                    int startRow,
                                    int startCol,
                                    int& currentMinorDx,
                                    int& currentMinorDy) {
    const int ROWS = m_mapDat.size();
    if (ROWS <= 0)
    {
        return {-1, -1};
    }
    const int COLS = m_mapDat.constBegin().value().size();
    if (COLS <= 0 || targetChars.empty())
    {
        return {-1, -1};
    }

    startRow = qBound(0, startRow, ROWS - 1);
    startCol = qBound(0, startCol, COLS - 1);

    MapDirectionVectors vectors = directionVectors(direction);
    DirectionVector currentMinor{currentMinorDx, currentMinorDy};
    if (!isSameAxis(currentMinor, vectors.minor))
    {
        currentMinor = vectors.minor;
    }

    std::unordered_set<std::string> targetSet(targetChars.begin(), targetChars.end());

    if (isVertical(vectors.major))
    {
        int col = startCol + currentMinor.dx;
        while (col >= 0 && col < COLS)
        {
            if (containsTarget(m_mapDat, targetSet, startRow, col))
            {
                currentMinorDx = currentMinor.dx;
                currentMinorDy = currentMinor.dy;
                return {startRow, col};
            }
            col += currentMinor.dx;
        }

        for (int row = startRow + vectors.major.dy;
             row >= 0 && row < ROWS;
             row += vectors.major.dy)
        {
            DirectionVector rowMinor = (qAbs(row - startRow) % 2 == 0)
                    ? currentMinor
                    : negateVector(currentMinor);
            int rowCol = rowMinor.dx > 0 ? 0 : COLS - 1;
            while (rowCol >= 0 && rowCol < COLS)
            {
                if (containsTarget(m_mapDat, targetSet, row, rowCol))
                {
                    currentMinorDx = rowMinor.dx;
                    currentMinorDy = rowMinor.dy;
                    return {row, rowCol};
                }
                rowCol += rowMinor.dx;
            }
        }
    }
    else
    {
        int row = startRow + currentMinor.dy;
        while (row >= 0 && row < ROWS)
        {
            if (containsTarget(m_mapDat, targetSet, row, startCol))
            {
                currentMinorDx = currentMinor.dx;
                currentMinorDy = currentMinor.dy;
                return {row, startCol};
            }
            row += currentMinor.dy;
        }

        for (int col = startCol + vectors.major.dx;
             col >= 0 && col < COLS;
             col += vectors.major.dx)
        {
            DirectionVector columnMinor = (qAbs(col - startCol) % 2 == 0)
                    ? currentMinor
                    : negateVector(currentMinor);
            int columnRow = columnMinor.dy > 0 ? 0 : ROWS - 1;
            while (columnRow >= 0 && columnRow < ROWS)
            {
                if (containsTarget(m_mapDat, targetSet, columnRow, col))
                {
                    currentMinorDx = columnMinor.dx;
                    currentMinorDy = columnMinor.dy;
                    return {columnRow, col};
                }
                columnRow += columnMinor.dy;
            }
        }
    }

    currentMinorDx = currentMinor.dx;
    currentMinorDy = currentMinor.dy;
    return {-1, -1};
}

// 对外接口：从设置的起点启动加工步进，并先计算第一段路径。
bool MapTest::startMapPathPlanning()
{
    if (m_mapData.isEmpty())
    {
        return false;
    }
    if (m_isFindingStartPoint)
    {
        QMessageBox::warning(this, QStringLiteral("路径规划"), QStringLiteral("正在寻起点，请等待完成后再开始路径规划。"));
        return false;
    }
    if (!m_hasFoundStartPoint)
    {
        QMessageBox::warning(this, QStringLiteral("路径规划"), QStringLiteral("请先点击“寻起点”，走到起点后再开始路径规划。"));
        return false;
    }

    MapDirectionVectors vectors = directionVectors(m_selectedDirection);
    m_currentMinorDx = vectors.minor.dx;
    m_currentMinorDy = vectors.minor.dy;
    m_pendingPathDisplayCells.clear();
    m_assistPathSourceCells.clear();
    clearFindStartRoute();

    if (m_hasStartPoint)
    {
        QPoint startDisplayCell = sourceToDisplayCell(m_startPointSource.x(), m_startPointSource.y());
        setSelectedCell(startDisplayCell.x(), startDisplayCell.y(), true, false);
    }

    std::unordered_map<std::string, int> checkedRows = checkedTargetRows();
    if (checkedRows.empty())
    {
        refreshMapViews();
        return false;
    }

    std::vector<std::string> targetChars;
    targetChars.reserve(checkedRows.size());
    for (const auto& checkedRow : checkedRows)
    {
        targetChars.push_back(checkedRow.first);
    }

    const MapData& searchMapData = currentMapData();
    if (!m_hasStartPoint && !containsTarget(searchMapData, targetChars, RecordCurrent_y, RecordCurrent_x))
    {
        std::pair<int, int> firstTarget = findFirstTargetPosition(searchMapData, targetChars, m_selectedDirection);
        if (firstTarget.first < 0 || firstTarget.second < 0)
        {
            refreshMapViews();
            return false;
        }
        setSelectedCell(firstTarget.second, firstTarget.first, true, false);
        setStartPointFromSelection();
    }

    refreshMapViews();

    if (!advanceSearchStep())
    {
        return false;
    }

    if (!m_pTimer->isActive())
    {
        setMapFunctionPanelRunning(true);
        m_pTimer->start(MapStepIntervalMs);
    }
    return true;
}

// UI包装：开始按钮调用统一公开接口。
void MapTest::on_start_pushButton_clicked()
{
    if (m_pTimer->isActive() && !m_isFindingStartPoint)
    {
        stopMapPathPlanning();
        return;
    }

    startMapPathPlanning();
}

// 定时器回调：每次推进一步，找不到目标时自动停止。
void MapTest::handleTimeout()
{
    if (m_isFindingStartPoint)
    {
        if (!advanceFindStartStep())
        {
            m_pTimer->stop();
            setMapFunctionPanelRunning(false);
        }
        return;
    }

    if (!advanceSearchStep())
    {
        m_pTimer->stop();
        setMapFunctionPanelRunning(false);
        return;
    }
}

// 寻起点时每次走一格，走到起点后才允许开始路径规划。
bool MapTest::advanceFindStartStep()
{
    bool reachedStart = false;
    if (!moveToNextFindStartCell(nullptr, &reachedStart))
    {
        return false;
    }

    return !reachedStart;
}

// 处理当前点并推进到下一步；需要借助路径时先走灰色借助格。
bool MapTest::advanceSearchStep()
{
    if (m_mapData.isEmpty())
    {
        return false;
    }

    if (m_pendingPathDisplayCells.isEmpty())
    {
        processCurrentCellIfChecked();
    }

    if (!moveToNextPlannedCell())
    {
        refreshMapViews();
        return false;
    }

    return true;
}
