#pragma execution_character_set("utf-8")
#include "maptest.h"
#include "ui_maptest.h"
#include "strCoding.h"
#include <queue>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <regex>
#include <algorithm>
#include <unordered_set>
MapTest::MapTest(QWidget* parent)
	: QMainWindow(parent)
	, ui(new Ui::MapTest)
{
	ui->setupUi(this);

    //ui->paintWidget->installEventFilter(this);
	ui->small_label->installEventFilter(this);
    ui->main_windows_label->installEventFilter(this);

    ui->main_windows_label->setScaledContents(true);
    ui->main_windows_label->setFixedWidth(mapwidth);
    ui->main_windows_label->setFixedHeight(mapheight);

    BigMapHorizontalslider = new RulerSlider(Qt::Horizontal,this);
    BigMapHorizontalslider->setFixedSize(mapwidth,35);//设置长宽
    BigMapHorizontalslider->setSingleStep(1);//刻度之间的距离
    BigMapHorizontalslider->setRulerSliderRange(0,200);//设置刻度范围
    BigMapHorizontalslider->setRulerSliderValue(200);//设置当前值
    BigMapHorizontalslider->move(0,mapheight);//移动

    BigMapVerticalslider = new RulerSlider(Qt::Vertical,this);
    BigMapVerticalslider->setFixedSize(35,mapheight);//设置长宽
    BigMapVerticalslider->setSingleStep(1);//刻度之间的距离
    BigMapVerticalslider->setRulerSliderRange(0,200);//设置刻度范围
    BigMapVerticalslider->setRulerSliderValue(200);//设置当前值
    BigMapVerticalslider->move(mapwidth,0);//移动


    SmallMapHorizontalslider = new RulerSlider(Qt::Horizontal,this);
    SmallMapHorizontalslider->setFixedSize(ui->small_label->width(),35);//设置长宽
    SmallMapHorizontalslider->setSingleStep(1);//刻度之间的距离
    SmallMapHorizontalslider->setRulerSliderRange(0,15);//设置刻度范围
    SmallMapHorizontalslider->setRulerSliderValue(15);//设置当前值
    SmallMapHorizontalslider->move(ui->small_label->geometry().x(),ui->small_label->height()+ui->small_label->geometry().y());//移动

    SmallMapVerticalslider = new RulerSlider(Qt::Vertical,this);
    SmallMapVerticalslider->setFixedSize(35,ui->small_label->height());//设置长宽
    SmallMapVerticalslider->setSingleStep(1);//刻度之间的距离
    SmallMapVerticalslider->setRulerSliderRange(0,15);//设置刻度范围
    SmallMapVerticalslider->setRulerSliderValue(15);//设置当前值
    SmallMapVerticalslider->move(ui->small_label->width()+ui->small_label->geometry().x(),ui->small_label->geometry().y());//移动

    m_drawingThread = new DrawingThread(ui->tableWidget,m_mapData,map_columns,map_rows,map_columns, map_rows,this);

    m_pTimer = new QTimer(this);

    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(handleTimeout()));


}

MapTest::~MapTest()
{
    delete ui;

    delete BigMapHorizontalslider;
    BigMapHorizontalslider=NULL;

    delete BigMapVerticalslider;
    BigMapVerticalslider=NULL;

    delete SmallMapHorizontalslider;
    SmallMapHorizontalslider=NULL;

    delete SmallMapVerticalslider;
    SmallMapVerticalslider=NULL;

    if (m_drawingThread)
        {
            m_drawingThread->quit();
            m_drawingThread->wait();
            delete m_drawingThread;
        }
}

//QString转Std::String
std::string QStringToStdString(const QString& qstr) {
    QByteArray byteArray = qstr.toUtf8();
    const char* utf8String = byteArray.constData();
    std::string stdString(utf8String);
    return stdString;
}


//获取CheckBox
QCheckBox* getCheckBox(QTableWidget*table,int row ,int column)
{
    QWidget* pWidget = 0;
    pWidget = table->cellWidget(row,column); //找到单元格
    QCheckBox *pCheckBox = 0;
    bool bNew = true;
    if(pWidget != 0) //
    {
        bNew = false;
    }
    if(bNew)
    {
        pWidget = new QWidget(table); //创建一个widget
        QHBoxLayout *hLayout = new QHBoxLayout(pWidget); //创建布局
        pCheckBox = new QCheckBox(pWidget);
        //根据objectName ,去拆解所属的tableWidget 、行、列
        pCheckBox->setObjectName(QString("%1_%2_%3_Itme").arg(table->objectName()).arg(row).arg(column));
        pCheckBox->setChecked(false);
        pCheckBox->setFont(table->font());
        pCheckBox->setFocusPolicy(Qt::NoFocus);
        pCheckBox->setStyle(QStyleFactory::create("fusion"));
        pCheckBox->setStyleSheet(QString(".QCheckBox {margin:3px;border:0px;}QCheckBox::indicator {width: %1px; height: %1px; }").arg(20));
        hLayout->addWidget(pCheckBox); //添加
        hLayout->setMargin(0); //设置边缘距离
        hLayout->setAlignment(pCheckBox, Qt::AlignCenter); //居中
        hLayout->setSpacing(0);
        pWidget->setLayout(hLayout); //设置widget的布局
        table->setCellWidget(row,column,pWidget);
    }
    QList<QCheckBox *> allCheckBoxs =  pWidget->findChildren<QCheckBox *>();
    if(allCheckBoxs.size() > 0)
        pCheckBox = allCheckBoxs.first();
    return pCheckBox;
}

//处理CheckBox
QCheckBox* setCheckBox(QTableWidget*table,int row ,int column,bool checkd)
{
    QCheckBox *check = getCheckBox(table,row,column);
    if(check != 0) //
    {
        check->setChecked(checkd);
    }
    QCheckBox::connect(check,&QCheckBox::stateChanged,[=]{
        QString objectName = check->objectName();
        QStringList nameList = objectName.split("_");//拆解
        if(nameList.count() == 4)
        {
            QString tableName = nameList.at(0);//表格名称
            int row = nameList.at(1).toInt();//行
            int column = nameList.at(2).toInt();//列
            bool checked = check->isChecked();//是否被选中

            //知道了表格、行、列，就可以执行我们所需要的操作了。。。
            qDebug() << QString("%1表第%2行第%3列是否被选中：%4")
                            .arg(tableName).arg(row).arg(column).arg(checked?"是":"否");

        }
    });
    return check;
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

QImage FullImage(const QString &imagePath,double StartPoint, double EndPoint)
{
    QImage image;
    //加载图片
    image.load(imagePath);

    //遍历每个像素点
    for(int i=StartPoint;i<EndPoint;i++)
    {
        for(int j=StartPoint;j<EndPoint;j++)
        {

           image.setPixelColor(i,j,Qt::white);

        }
    }

    //保存为新图片
    image.save("./chip.png");
    return image;

}

// 填充图像的指定矩形区域
void fillImageRect(const QString &imagePath, const QRectF& rect, const QColor& color)
{
    QImage image;
    //加载图片
    image.load(imagePath);
    // 创建一个画家对象，用于在图像上进行绘制操作
    QPainter painter(&image);

    // 设置填充颜色
    painter.setBrush(color);

    // 绘制填充区域
    painter.fillRect(rect, painter.brush());

    // 保存填充后的图像
    image.save(imagePath);
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

        qDebug() << "Most frequent pattern length:" << maxCountLength;

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



//    int startIdx = middleLine.size() / 2; // 从中间行的1/2位置开始计算空格规律

//    QByteArray pattern;

//    // 提取中间行的空格字符
//    for (int i = startIdx; i < middleLine.size(); i++) {
//        char ch = middleLine[i];
//        if (ch == ' ') {
//            pattern.append(ch);
//        }
//    }

//    // 删除没有与中间空格规律相同的行
//    for (int i = 0; i < mapData.size(); i++) {
//        QByteArray line = mapData[i];

//        if (line == middleLine) {
//            continue; // 跳过中间行
//        }

//        QByteArray linePattern;

//        // 提取行的空格字符，从1/2位置开始
//        for (int j = startIdx; j < line.size(); j++) {
//            char ch = line[j];
//            if (ch == ' ') {
//                linePattern.append(ch);
//            }
//        }

//        if (linePattern != pattern) {
//            mapData.remove(i);
//            i--; // 由于删除了一行，需要调整索引
//        }
//    }




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

//如果不是txt文件转化，获取新的路径
string getNewFilePath(string &filename)
{
    int ROW,COLUMN;

    std::ifstream file(filename); // 替换为您的文件路径

    if (file.is_open())
    {
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        std::regex patternROW("<ROW_COUNT>(.*)</ROW_COUNT>");
        std::regex patternCOLUMN("<COLUMN_COUNT>(.*)</COLUMN_COUNT>");
        std::regex patternWAFER("<WAFER_MAP>(.*)</WAFER_MAP>");

        std::smatch match;

        if (std::regex_search(content, match, patternROW))
        {
            std::string data = match.str(1);
            ROW = std::stoi(data);
            std::cout << "提取到的数据为: " << data << std::endl;
        }
        else {
            std::cout << "未找到匹配的内容。" << std::endl;
        }

        if (std::regex_search(content, match, patternCOLUMN))
        {
            std::string data = match.str(1);
            COLUMN = std::stoi(data);
            std::cout << "提取到的数据为: " << data << std::endl;
        }

        else
        {
            std::cout << "未找到匹配的内容。" << std::endl;
        }


        if (std::regex_search(content, match, patternWAFER))
        {
            std::string data = match.str(1);
            std::string strFileName = filename.substr(filename.find_last_of("\\") + 1);
            size_t num = strFileName.find_last_of('.');
            if (num > 0)
            {
                strFileName = strFileName.substr(0, num);
            }
             std::ofstream outputFile(strFileName + ".txt", std::ios::binary);

             if (outputFile.is_open())
             {
                 for (size_t i = 0; i < data.length(); i += COLUMN)
                 {
                     std::string line = data.substr(i, COLUMN);
                     outputFile.write((line+'\n').c_str(), line.length()+1);
                 }
                 outputFile.flush(); // 刷新缓冲区
                 outputFile.close();
                 std::cout << "Content has been written to the file successfully." << std::endl;
             }
             else
             {
                 std::cout << "Unable to open the file." << std::endl;

             }

            return(strFileName+".txt");
        }

        else
        {
            std::string strFileName = filename.substr(filename.find_last_of("\\") + 1);
            size_t num = strFileName.find_last_of('.');
            if (num > 0)
            {
                strFileName = strFileName.substr(0, num);
            }

            std::ofstream targetFile(strFileName + ".txt", std::ios::binary);

            targetFile << content; // 将源文件的内容复制到目标文件中
            targetFile.close();
            return(strFileName+".txt");
        }
    }

    else
    {
        std::cout << "无法打开文件！" << std::endl;
    }

    file.close();
    return filename;
}

//读取Txt按钮
void MapTest::on_read_pushButton_clicked()
{
    if ( m_drawingThread&&m_drawingThread->isRunning())
    {
        // Drawing is already in progress
        return;
    }
	m_mapData.clear();

    filTexyename = QFileDialog::getOpenFileName(this, "打开晶元图", "./",tr("*")).toLocal8Bit().constData();

    std::string stdstr = QStringToStdString(filTexyename);

    filTexyename=QString::fromLocal8Bit(getNewFilePath(stdstr).c_str()).toLocal8Bit().constData();

    ReadMapTxtFile();

    ui->label->setText(" 读取完成！! !");

}

//计算出现频率最高的列数
QPair<int, int> currenceLength(const QString &filePath, int &mostFrequentLength) {
    QMap<int, int> lengthCounts;
        QMap<int, int> lengthFirstLine;
        QMap<int, int> lengthLastLine;

        // 打开文件
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "无法打开文件：" << file.errorString();
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
void MapTest::ReadMapTxtFile()
{
	QFile file(filTexyename);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return;

        int mostFrequentColumn;
        QPair<int, int> result = currenceLength(filTexyename, mostFrequentColumn);
        qDebug() << "出现频率最高的列数是：" << mostFrequentColumn;
        qDebug() << "第一次出现的行号：" << result.first;
        qDebug() << "最后一次出现的行号：" << result.second;

	QVector<QByteArray> fileText;
	int nMaxSize = 0;
//	while (!file.atEnd())
//	{
//		QByteArray line = file.readLine();
//		if (nMaxSize < line.size())
//		{
//			nMaxSize = line.size();
//		}
//		fileText.push_back(line);
//	}
    QVector<QByteArray> mapData;

//	for (auto it : fileText)
//	{
//		if (it.size() == nMaxSize)
//		{
//			mapData.push_back(it);
//		}
//	}

    int currentLine = 1;
    while (!file.atEnd() && currentLine <= result.second) {
            QByteArray line = file.readLine();
            if (currentLine >= result.first) {
                mapData.append(line);
            }
            ++currentLine;
        }


    std::string rowString;
    // 将行数据转换为 QString
    for (const auto& ch : mapData[mapData.size() / 2])
    {
        QString qstr = QString::fromStdString(std::string(1, ch));
        rowString += qstr.toStdString();
    }

    if (hasSpacePattern(rowString))
    {
        removeLinesWithoutSpacePattern(mapData);

        int findFirstAllSpaces=findFirstAllSpacesColumn(mapData);
        removeColumns(mapData,findFirstAllSpaces);

        removeWhitespaceRowsAndColumns(mapData);
    }
    else
    {
        if(hasSpace(rowString))
        {
            int startIndex = getLastColumnStartIndex(mapData);
            removeRows(mapData,startIndex);

            int findFirstAllSpaces=findFirstAllSpacesColumn(mapData);
            removeColumns(mapData,findFirstAllSpaces);

            removeWhitespaceRowsAndColumns(mapData);

        }
    }

    // 调用函数找到符合条件的子串并存储在容器中
    std::vector<std::string> results = findAndStore(rowString);

    int charnum=1;
    // 打印容器中的结果
    if (!results.empty())
    {
        charnum=results[0].length();
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
               for (int k = 0; k < charnum-1 && j + k < mapData.at(i).size(); k++)
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

    // 获取 m_mapData 中所有不同字符串的种类

    std::vector<std::string> stringTypes;

    for (const auto& entry : m_stringCount)
    {
        stringTypes.push_back(entry.first);
    }

	int EffectiveNum = 0;//得到包含的字符数

	ui->tableWidget->setRowCount(0);
    ui->tableWidget->setEditTriggers(QTableWidget::NoEditTriggers);//不能编辑
    for (int j = 0; j < stringTypes.size(); j++)
    {
            QColor clr(rand() % 256, rand() % 256, rand() % 256);//获取随机颜

            ui->tableWidget->setRowCount(EffectiveNum+1);//新增一行

            setCheckBox(ui->tableWidget,EffectiveNum,0,false);

            ui->tableWidget->setItem(EffectiveNum, 1, new QTableWidgetItem(QString::fromStdString(stringTypes[j]))); //在新建行插入ascll

            ui->tableWidget->setItem(EffectiveNum, 2, new QTableWidgetItem(QString::number(m_stringCount[stringTypes[j]]))); //在新建行插入 某个ascll数量
            ui->tableWidget->setItem(EffectiveNum, 3, new QTableWidgetItem(QString::number(m_stringCount[stringTypes[j]])));

            ui->tableWidget->setItem(EffectiveNum, 4, new QTableWidgetItem(clr.name())); //设置新建行的背景色
            ui->tableWidget->item(EffectiveNum, 4)->setBackground(clr);

			EffectiveNum++;

            cout << stringTypes[j]<< ":" << m_stringCount[stringTypes[j]] << endl; //输出相应的字符和出现次数

    }
    //设置已加工
    ui->tableWidget->setRowCount(EffectiveNum+1);//新增一行


    ui->tableWidget->setItem(EffectiveNum, 0, new QTableWidgetItem("已加工"));

    ui->tableWidget->setItem(EffectiveNum, 1, new QTableWidgetItem(QString('#'))); //在新建行插入ascll

    ui->tableWidget->setItem(EffectiveNum, 2, new QTableWidgetItem("0")); //在新建行插入 某个ascll数量

    ui->tableWidget->setItem(EffectiveNum, 4, new QTableWidgetItem("#ffffff")); //设置新建行的背景色

    ui->tableWidget->item(EffectiveNum, 4)->setBackground(Qt::white);


    map_columns = m_mapData[0].size();

	map_rows = m_mapData.count();


    BigMapHorizontalslider->setFixedSize(mapwidth,35);//设置长宽
    BigMapHorizontalslider->setSingleStep(1);//刻度之间的距离
    BigMapHorizontalslider->setRulerSliderRange(0,map_columns);//设置刻度范围
    BigMapHorizontalslider->setRulerSliderValue(map_columns);//设置当前值
    BigMapHorizontalslider->move(0,mapheight);//移动


    BigMapVerticalslider->setFixedSize(35,mapheight);//设置长宽
    BigMapVerticalslider->setSingleStep(1);//刻度之间的距离
    BigMapVerticalslider->setRulerSliderRange(0,map_rows);//设置刻度范围
    BigMapVerticalslider->setRulerSliderValue(map_rows);//设置当前值
    BigMapVerticalslider->move(mapwidth,0);//移动

    m_scaleFactor=1;
    rodegrees = 0;
    Paint();
    update(); //刷新显示

}


//事件过滤器
bool MapTest::eventFilter(QObject* watched, QEvent* event)
{
//    if (watched == ui->paintWidget && event->type() == QEvent::Paint)
//        if (event->type() == QEvent::Paint)
//            Paint();

	if (watched == ui->small_label && event->type() == QEvent::Paint)
		Paint_Secondary();


    if (watched == ui->centralwidget && event->type() == QEvent::Paint)
        Paint_rectangle();

    QLineEdit* editor = dynamic_cast<QLineEdit*>(watched);
    if (editor && event->type() == QEvent::FocusOut)
    {
        // 编辑器失去焦点，执行输入验证
        QString text = editor->text();
        if (text.isEmpty())
        {
            // 输入为空，发出警告或禁止用户离开单元格
            QMessageBox::warning(this, "Warning", "不能设置为空!");
            qDebug() << "Input cannot be empty!";
            // 例如，您可以显示一个警告对话框，或者强制重新编辑该单元格
        }
        else if (text.length() > 1)
        {
            // 输入超过一个字符，发出警告或截取第一个字符
            qDebug() << "Input should be a single character!";
            // 例如，您可以显示一个警告对话框，并截取第一个字符
            editor->setText(text.left(1));
        }
    }

	return QWidget::eventFilter(watched, event);

}

//画晶元
void MapTest::Paint()
{

	if (m_mapData.isEmpty()) { return; }

    double win_width = mapwidth / (double)map_columns;
    double win_height = mapheight /(double) map_rows;

    ui->x_spinBox->setMaximum(map_columns);
    ui->y_spinBox->setMaximum(map_rows - 1);
    rectangleLabel.setRectangle(win_width*15,win_height*15);
    // Create a new drawing thread

     m_drawingThread->UpdateCurrentlyParam(ui->tableWidget,m_mapData,win_width,win_height,map_columns, map_rows);

    // Connect the finished signal to handle the result
    connect(m_drawingThread, &DrawingThread::finished, this, [this]() {
        ui->tableWidget->setEnabled(true);

        // Update UI with the generated image
        QString	filename = "./chip.png";

        read_image(filename);

    });

    // Disable the table widget while drawing
   ui->tableWidget->setEnabled(false);

    // Start the drawing thread
    m_drawingThread->start();



}

//画小窗口
void MapTest::Paint_Secondary()
{

    if (IsDrawSmallMap || IsClickSmallMap)
    {

        mylabel.UpdateRectangle();

        QPixmap pixmap(ui->small_label->height(), ui->small_label->width());
        QPainter painter(&pixmap);
        //painter.begin(&pixmap);
        pixmap.fill(Qt::white);
        QPen pen = painter.pen();
        pen.setWidth(1);

        ui->x_pos_label->setText("X坐标：" + QString::number(RecordCurrent_x+BigMapX));
        ui->y_pos_label->setText("Y坐标：" + QString::number(RecordCurrent_y+BigMapY));

        QElapsedTimer mstimer;//计时器
        mstimer.start();
        QMap<int, char> new_mapData;


        int cellSize = 18; // 单元格的大小
        int padding = 6; // 单元格之间的间距


        int centerX = RecordCurrent_x+BigMapX; // 矩形的中心X坐标
        int centerY = RecordCurrent_y+BigMapY; // 矩形的中心Y坐标


        int rectWidth=ui->small_label->width()/(cellSize + padding) ;// 矩形的宽度个数
        int rectHeight= ui->small_label->height()/(cellSize + padding); // 矩形的高度个数


        int startX = centerX - rectWidth / 2; // 矩形左上角的X坐标
        int startY = centerY - rectHeight / 2; // 矩形左上角的Y坐标


        int m_top = 0;

        // 使用无序映射进行数据查找
        std::unordered_map<std::string, QColor> colorMap;
        for (int n = 0; n < ui->tableWidget->rowCount(); n++)
        {
            QString data = ui->tableWidget->model()->index(n, 1).data().toString();
            std::string ch = data.toStdString();
            QColor color = QColor(ui->tableWidget->model()->index(n, 4).data().toString());
            colorMap[ch] = color;
        }


        for (int y = startY; y <= startY + rectHeight; y++)
        {
            QMap<int, string> new_mapData;
            if (rodegrees == 0) {
                new_mapData = m_mapData.value(y);
            }
            else {
                new_mapData = rotate_m_mapData.value(y);
            }

            int m_left = 0;
            for (int x = startX; x <= startX + rectWidth; x++)
            {
                string ASCII = new_mapData.value(x);
                // 从无序映射中查找颜色
                auto iter = colorMap.find(ASCII);
                //for (int n = 0; n < ui->tableWidget->rowCount(); n++)
                {
                    if (iter != colorMap.end())
                    {
                        QRect rect(m_left, m_top, cellSize, cellSize);
                        painter.fillRect(rect, iter->second);
                        painter.setPen(Qt::black);
                        painter.drawText(rect, Qt::AlignCenter, QString::fromStdString(ASCII));
                        //break;
                    }
                    else if (ASCII == " ")
                    {
                        QRect rect(m_left, m_top, cellSize, cellSize);
                        painter.fillRect(rect, Qt::white);
                        //break;
                    }
                    else
                    {
                        QRect rect(m_left, m_top, cellSize, cellSize);
                        painter.fillRect(rect, Qt::white);
                        painter.setPen(Qt::black);
                        painter.drawText(rect, Qt::AlignCenter, QString::fromStdString(ASCII));
                    }
                }

                m_left += cellSize + padding;
            }

            m_top += cellSize + padding;
        }


        painter.end();
        ui->small_label->setPixmap(pixmap);

        //显示执行时间
        double time = (double)mstimer.nsecsElapsed() / (double)1000000;
        QString str = QString::number(time, 'f', 2);
        ui->label->setText("运行时间: " + str + "ms");

        IsDrawSmallMap = false;
        IsClickSmallMap = false;

	}

}

//画矩形框
void MapTest::Paint_rectangle()
{

    //QPainter painter(ui->main_windows_label);

    //painter.setPen(QPen(Qt::red, 2));

}

//读取图片 并显示
void MapTest::read_image(QString filename)
{
	if (filename.isEmpty())
	{
		return;
	}   
    QPixmap Pixmap=LoadImageFunction(filename,ui->main_windows_label->size());
    ui->main_windows_label->setPixmap(Pixmap);
    Image = Pixmap.toImage();
}

//鼠标滚动
void MapTest::wheelEvent(QWheelEvent *event)
{
    // 获取滚轮滚动的角度
     int delta = event->angleDelta().y();
    // 向上滚动，放大
    if (delta > 0 )
   {
        if(m_scaleFactor==1.1)return;



        m_scaleFactor=1.1;
        QPixmap pixmap(ui->main_windows_label->height(), ui->main_windows_label->width());
        QPainter painter(&pixmap);

        pixmap.fill(Qt::white);
        QPen pen = painter.pen();
        pen.setWidth(1);

        QMap<int, char> new_mapData;


        int cellSize = 18; // 单元格的大小
        int padding = 6; // 单元格之间的间距

        int centerX = RecordCurrent_x; // 矩形的中心X坐标
        int centerY = RecordCurrent_y; // 矩形的中心Y坐标


        double rectWidth=ui->main_windows_label->width()/(cellSize + padding) ;// 矩形的宽度个数
        double rectHeight= ui->main_windows_label->height()/(cellSize + padding); // 矩形的高度个数

        int startX = centerX - int(rectWidth / 2); // 矩形左上角的X坐标
        int startY = centerY - int(rectHeight / 2); // 矩形左上角的Y坐标

        int m_top = 0;

        rectangleLabel.setRectangle(30,30);

        rectangleLabel.UpdateRectangle(QRectF((mapwidth)/2-24,(mapheight)/2-24,30,30));

        // 使用无序映射进行数据查找
        std::unordered_map<std::string, QColor> colorMap;
        for (int n = 0; n < ui->tableWidget->rowCount(); n++)
        {
            QString data = ui->tableWidget->model()->index(n, 1).data().toString();
            std::string ch = data.toStdString();
            QColor color = QColor(ui->tableWidget->model()->index(n, 4).data().toString());
            colorMap[ch] = color;
        }


        for (int y = startY; y <= startY + rectWidth; y++)
        {
            QMap<int, string> new_mapData;
            if (rodegrees == 0) {
                new_mapData = m_mapData.value(y);
            }
            else {
                new_mapData = rotate_m_mapData.value(y);
            }

            int m_left = 0;
            for (int x = startX; x <= startX + rectHeight; x++)
            {
                string ASCII = new_mapData.value(x);
                // 从无序映射中查找颜色
                auto iter = colorMap.find(ASCII);
                //for (int n = 0; n < ui->tableWidget->rowCount(); n++)
                {
                    if (iter != colorMap.end())
                    {
                        QRect rect(m_left, m_top, cellSize, cellSize);
                        painter.fillRect(rect, iter->second);
                        painter.setPen(Qt::black);
                        painter.drawText(rect, Qt::AlignCenter, QString::fromStdString(ASCII));
                        //break;
                    }
                    else if (ASCII == " ")
                    {
                        QRect rect(m_left, m_top, cellSize, cellSize);
                        painter.fillRect(rect, Qt::white);
                        //break;
                    }
                    else
                    {
                        QRect rect(m_left, m_top, cellSize, cellSize);
                        painter.fillRect(rect, Qt::white);
                        painter.setPen(Qt::black);
                        painter.drawText(rect, Qt::AlignCenter, QString::fromStdString(ASCII));
                    }
                }

                m_left += cellSize + padding;
            }

            m_top += cellSize + padding;
        }


        painter.end();
        ui->main_windows_label->setPixmap(pixmap);
        }
    else if (delta < 0)
    {
        RecordCurrent_x+=BigMapX;
        RecordCurrent_y+=BigMapY;
        BigMapX=0;
        BigMapY=0;

        if(rodegrees == 0 || rodegrees == 180)
        {
          rectangleLabel.setRectangle(mapwidth/(double)map_columns*15,mapheight/(double)map_rows*15);
          rectangleLabel.UpdateRectangle(QRectF((RecordCurrent_x-7)*mapwidth/(double)map_columns,(RecordCurrent_y-7)*mapheight/(double)map_rows,mapheight/(double)map_columns*15,mapheight/(double)map_rows*15));
        }
        else if(rodegrees == 90 || rodegrees == 270)
        {
           rectangleLabel.setRectangle(mapwidth/(double)map_rows*15,mapheight/(double)map_columns*15);
           rectangleLabel.UpdateRectangle(QRectF((RecordCurrent_x -7)*mapwidth/(double)map_rows,(RecordCurrent_y -7)*mapheight/(double)map_columns,mapwidth/(double)map_rows*15,mapheight/(double)map_columns*15));
        }
        m_scaleFactor=1;
        ui->main_windows_label->setPixmap(QPixmap::fromImage(Image));
    }
    else
    {
        event->ignore();
        return;
    }
}
//鼠标拖拽
void MapTest::mouseMoveEvent(QMouseEvent *e)
{
    if (m_scaleFactor!=1.1)
    {
      return;
    }
    setCursor(Qt::ClosedHandCursor);
    QPoint delta = e->pos() - m_dragStartPosition;


    RecordCurrent_x+=delta.x()/24;
    RecordCurrent_y+=delta.y()/24;

    if(RecordCurrent_x<0)
    {
        RecordCurrent_x=0;
    }
    else if(RecordCurrent_x>map_columns)
    {
        RecordCurrent_x=map_columns;
    }

    if(RecordCurrent_y<0)
    {
        RecordCurrent_y=0;
    }
    else if(RecordCurrent_y>map_rows)
    {
        RecordCurrent_y=map_rows;
    }

    QPixmap pixmap(ui->main_windows_label->height(), ui->main_windows_label->width());
    QPainter painter(&pixmap);

    pixmap.fill(Qt::white);
    QPen pen = painter.pen();
    pen.setWidth(1);

    QMap<int, char> new_mapData;


    int cellSize = 18; // 单元格的大小
    int padding = 6; // 单元格之间的间距

    int centerX = RecordCurrent_x; // 矩形的中心X坐标
    int centerY = RecordCurrent_y; // 矩形的中心Y坐标


    double rectWidth=ui->main_windows_label->width()/(cellSize + padding) ;// 矩形的宽度个数
    double rectHeight= ui->main_windows_label->height()/(cellSize + padding); // 矩形的高度个数

    int startX = centerX - int(rectWidth / 2); // 矩形左上角的X坐标
    int startY = centerY - int(rectHeight / 2); // 矩形左上角的Y坐标

    int m_top = 0;

    // 使用无序映射进行数据查找
    std::unordered_map<std::string, QColor> colorMap;
    for (int n = 0; n < ui->tableWidget->rowCount(); n++)
    {
        QString data = ui->tableWidget->model()->index(n, 1).data().toString();
        std::string ch = data.toStdString();
        QColor color = QColor(ui->tableWidget->model()->index(n, 4).data().toString());
        colorMap[ch] = color;
    }


    for (int y = startY; y <= startY + rectWidth; y++)
    {
        QMap<int, string> new_mapData;
        if (rodegrees == 0) {
            new_mapData = m_mapData.value(y);
        }
        else {
            new_mapData = rotate_m_mapData.value(y);
        }

        int m_left = 0;
        for (int x = startX; x <= startX + rectHeight; x++)
        {
            string ASCII = new_mapData.value(x);
            // 从无序映射中查找颜色
            auto iter = colorMap.find(ASCII);
            //for (int n = 0; n < ui->tableWidget->rowCount(); n++)
            {
                if (iter != colorMap.end())
                {
                    QRect rect(m_left, m_top, cellSize, cellSize);
                    painter.fillRect(rect, iter->second);
                    painter.setPen(Qt::black);
                    painter.drawText(rect, Qt::AlignCenter, QString::fromStdString(ASCII));
                    //break;
                }
                else if (ASCII == " ")
                {
                    QRect rect(m_left, m_top, cellSize, cellSize);
                    painter.fillRect(rect, Qt::white);
                    //break;
                }
                else
                {
                    QRect rect(m_left, m_top, cellSize, cellSize);
                    painter.fillRect(rect, Qt::white);
                    painter.setPen(Qt::black);
                    painter.drawText(rect, Qt::AlignCenter, QString::fromStdString(ASCII));
                }
            }

            m_left += cellSize + padding;
        }

        m_top += cellSize + padding;
    }


    painter.end();
    ui->main_windows_label->setPixmap(pixmap);

    IsClickSmallMap = true;
    update();
}
//鼠标按下
void MapTest::mousePressEvent(QMouseEvent* e)
{

    QWidget::mousePressEvent(e);
    setCursor(Qt::ArrowCursor); // 鼠标移回区域时恢复箭头光标
    //if(m_scaleFactor!=1)
       //return;
    Mouse_x = e->pos().x();
    Mouse_y = e->pos().y();

    qDebug() << Mouse_x << Mouse_y;

    QRect mainLabelGeometry = ui->main_windows_label->geometry();
    QRect smallLabelGeometry = ui->small_label->geometry();

    if (Mouse_x > mainLabelGeometry.x()
        && Mouse_y > mainLabelGeometry.y()
        && Mouse_x < mainLabelGeometry.x() + mainLabelGeometry.width()
        && Mouse_y < mainLabelGeometry.y() + mainLabelGeometry.height())
    {
        m_dragStartPosition = e->pos();
        BigMapX=0;
        BigMapY=0;

        if(m_scaleFactor==1.1)
        {
            int  Size=24;
            BigMapX = (Mouse_x - mainLabelGeometry.x()) / Size- mapwidth/Size/2;
            BigMapY = (Mouse_y - mainLabelGeometry.y()) / Size- mapheight/Size/2;

        }
        else if (rodegrees == 0 || rodegrees == 180)
        {

            RecordCurrent_x = (Mouse_x - mainLabelGeometry.x()) / (mapwidth / (double)map_columns);
            RecordCurrent_y = (Mouse_y - mainLabelGeometry.y()) / (mapheight / (double)map_rows);
        }
        else if (rodegrees == 90 || rodegrees == 270)
        {
            RecordCurrent_x = (Mouse_x - mainLabelGeometry.x()) / (mapwidth / (double)map_rows);
            RecordCurrent_y = (Mouse_y - mainLabelGeometry.y()) / (mapheight / (double)map_columns);
        }

        SmallMapHorizontalslider->setRulerSliderRange(RecordCurrent_x +BigMapX- 7, RecordCurrent_x+BigMapX + 8); // 设置刻度范围
        SmallMapHorizontalslider->setRulerSliderValue(15); // 设置当前值

        SmallMapVerticalslider->setRulerSliderRange(RecordCurrent_y+BigMapY - 7, RecordCurrent_y +BigMapY+ 8); // 设置刻度范围
        SmallMapVerticalslider->setRulerSliderValue(15); // 设置当前值

        ui->x_spinBox->setValue(RecordCurrent_x+BigMapX);
        ui->y_spinBox->setValue(RecordCurrent_y+BigMapY);

        IsDrawSmallMap = true;
        update();
    }

    if (Mouse_x > smallLabelGeometry.x()
        && Mouse_y > smallLabelGeometry.y()
        && Mouse_x < smallLabelGeometry.x() + smallLabelGeometry.width()
        && Mouse_y < smallLabelGeometry.y() + smallLabelGeometry.height())
    {
        int SmallMapX = (Mouse_x - smallLabelGeometry.x()) / 24;
        int SmallMapY = (Mouse_y - smallLabelGeometry.y()) / 24;
        int maxRow, maxCol;

        if (rodegrees == 0 || rodegrees == 180)
        {
            maxRow = map_columns;
            maxCol = map_rows;
        }
        else if (rodegrees == 90 || rodegrees == 270)
        {
            maxRow = map_rows;
            maxCol = map_columns;
        }

        if ((RecordCurrent_x + SmallMapX - 7) < maxRow && (RecordCurrent_y + SmallMapY - 7) < maxCol)
        {
            ui->x_pos_label->setText("X坐标：" + QString::number(RecordCurrent_x + SmallMapX - 7));
            ui->y_pos_label->setText("Y坐标：" + QString::number(RecordCurrent_y + SmallMapY - 7));
        }
        ui->x_spinBox->setValue(RecordCurrent_x + SmallMapX - 7);
        ui->y_spinBox->setValue(RecordCurrent_y + SmallMapY - 7);
        ui->small_label->update();
    }
}
//确认
void MapTest::on_sure_pushButton_clicked()
{

      RecordCurrent_x = ui->x_spinBox->text().toInt();
      RecordCurrent_y = ui->y_spinBox->text().toInt();

      if(rodegrees == 0 || rodegrees == 180)
      {
        rectangleLabel.UpdateRectangle(QRectF((RecordCurrent_x-7)*mapwidth/(double)map_columns,(RecordCurrent_y-7)*mapheight/(double)map_rows,mapheight/(double)map_columns*15,mapheight/(double)map_rows*15));
      }
      else if(rodegrees == 90 || rodegrees == 270)
      {
         rectangleLabel.UpdateRectangle(QRectF((RecordCurrent_x -7)*mapwidth/(double)map_rows,(RecordCurrent_y -7)*mapheight/(double)map_columns,mapwidth/(double)map_rows*15,mapheight/(double)map_columns*15));
      }

      SmallMapHorizontalslider->setRulerSliderRange(RecordCurrent_x-7,RecordCurrent_x+8);//设置刻度范围
      SmallMapHorizontalslider->setRulerSliderValue(15);//设置当前值

      SmallMapVerticalslider->setRulerSliderRange(RecordCurrent_y-7,RecordCurrent_y+8);//设置刻度范围
      SmallMapVerticalslider->setRulerSliderValue(15);//设置当前值

      std::unordered_map<string, int> checkedRows;

      for (int row = 0; row < ui->tableWidget->rowCount(); ++row)
      {
         if(getCheckBox(ui->tableWidget,row,0)->checkState())
         {

              QString data = ui->tableWidget->model()->index(row, 1).data().toString();
              //char ch = data.at(0).toLatin1();
              checkedRows[data.toStdString()] = row;
         }
      }

      auto Value= checkedRows.find(m_mapData[RecordCurrent_y][RecordCurrent_x]);

      if (Value!= checkedRows.end())
      {
          // 值存在于 checkedRows 中
//          QString data =ui->tableWidget->model()->index(ui->tableWidget->rowCount()-1, 1).data().toString();
//          char ch = data.isEmpty() ? ' ' : data.at(0).toLatin1();
          m_mapData[RecordCurrent_y][RecordCurrent_x]='#';


          QTableWidgetItem* item = ui->tableWidget->item(Value->second, 3); // 获取单元格的项

          if (item != nullptr)
          {
              QString value = item->text(); // 获取项的文本值
              int numericValue = value.toInt(); // 将文本值转换为整数

              // 对数值进行减一操作
              numericValue--;

              // 将减少后的数值转换回字符串，并设置回单元格
              item->setText(QString::number(numericValue));

              // 刷新表格显示
              ui->tableWidget->viewport()->update();

          }

          else
          {
              // 单元格为空，没有项
              std::cout << "Cell is empty." << std::endl;
          }
          //fillImageRect("./chip.png",QRectF((RecordCurrent_x)*mapwidth/(double)map_columns,(RecordCurrent_y)*mapheight/(double)map_rows,mapheight/(double)map_columns,mapheight/(double)map_rows),Qt::white);
          //read_image("./chip.png");

          // 获取标签原图并复制
          QPixmap pixmap = ui->main_windows_label->pixmap() ?
                           ui->main_windows_label->pixmap()->copy() :
                           QPixmap(ui->main_windows_label->size());

          double cellWidth = mapwidth / (double)map_columns;
          double cellHeight = mapheight / (double)map_rows;

          QRectF updateRect(
              RecordCurrent_x * cellWidth,
              RecordCurrent_y * cellHeight,
              cellWidth,
              cellHeight
          );

          QPainter painter(&pixmap);
          painter.fillRect(updateRect, Qt::white);  // 仅填充目标区域
          painter.end();

          ui->main_windows_label->setPixmap(pixmap);

      }
      else
      {
          // 值不存在于 checkedRows 中
          std::cout << "Value not found in checkedRows." << std::endl;
      }


     IsClickSmallMap = true;
     update();

}
//旋转
void MapTest::on_rotate_clicked()
{
    if ( m_drawingThread&&m_drawingThread->isRunning())
    {
        // Drawing is already in progress
        return;
    }
	if (m_mapData.isEmpty()) { return; }
    // 使用取模运算简化旋转角度的计算

    // 调整旋转角度
    rodegrees = (rodegrees + 90) % 360;
    // 创建 QTransform 对象并设置旋转角度
	QMatrix matrix;
    matrix.rotate(90);

    //ui->main_windows_label->setPixmap(QPixmap("./chip.png").transformed(matrix, Qt::SmoothTransformation));

    // 应用变换
    QPixmap transformedPixmap = QPixmap("./chip.png").transformed(matrix, Qt::SmoothTransformation);

    // 设置到标签上并保存旋转后的图像
    ui->main_windows_label->setPixmap(transformedPixmap);

    // 保存旋转后的图像
    if (!transformedPixmap.save("./chip.png")) {
        qWarning() << "Failed to save image to ./chip.png";
    }
    Image = transformedPixmap.toImage();
	if (rodegrees == 0)//正常矩阵
	{
		rotate_m_mapData.clear();
		for (int i = 0; i < map_rows; i++)
		{
			for (int j = 0; j < map_columns; j++)
			{
				rotate_m_mapData[i][j] = m_mapData[i][j];
			}
		}
        rectangleLabel.setRectangle(mapwidth/(double)map_columns*15,mapheight/(double)map_rows*15);

        BigMapHorizontalslider->setRulerSliderRange(0,map_columns);//设置刻度范围
        BigMapHorizontalslider->setRulerSliderValue(map_columns);//设置当前值

        BigMapVerticalslider->setRulerSliderRange(0,map_rows);//设置刻度范围
        BigMapVerticalslider->setRulerSliderValue(map_rows);//设置当前值
	}

	else if (rodegrees == 90)//顺时针旋转90度矩阵
	{
		rotate_m_mapData.clear();
		for (int i = 0; i < map_rows; i++)
		{
			for (int j = 0; j < map_columns; j++)
			{
				rotate_m_mapData[j][map_rows - 1 - i] = m_mapData[i][j];
			}
		}
		ui->x_spinBox->setMaximum(map_rows - 1);
        ui->y_spinBox->setMaximum(map_columns - 1);
        rectangleLabel.setRectangle(mapwidth/(double)map_rows*15,mapheight/(double)map_columns*15);


        BigMapHorizontalslider->setRulerSliderRange(0,map_rows);//设置刻度范围
        BigMapHorizontalslider->setRulerSliderValue(map_rows);//设置当前值

        BigMapVerticalslider->setRulerSliderRange(0,map_columns);//设置刻度范围
        BigMapVerticalslider->setRulerSliderValue(map_columns);//设置当前值

	}
	else if (rodegrees == 180)//旋转180度矩阵
	{
		rotate_m_mapData.clear();
		for (int i = 0; i < map_rows; i++)
		{
			for (int j = 0; j < map_columns; j++)
			{
				rotate_m_mapData[i][j] = m_mapData[map_rows - 1 - i][map_columns - 1 - j];
			}
		}
        ui->x_spinBox->setMaximum(map_columns);
		ui->y_spinBox->setMaximum(map_rows - 1);
        rectangleLabel.setRectangle(mapwidth/(double)map_columns*15,mapheight/(double)map_rows*15);

        BigMapHorizontalslider->setRulerSliderRange(0,map_columns);//设置刻度范围
        BigMapHorizontalslider->setRulerSliderValue(map_columns);//设置当前值

        BigMapVerticalslider->setRulerSliderRange(0,map_rows);//设置刻度范围
        BigMapVerticalslider->setRulerSliderValue(map_rows);//设置当前值
	}

	else if (rodegrees == 270)//旋转270度矩阵
	{
		rotate_m_mapData.clear();
		for (int i = 0; i < map_rows; i++)
		{
			for (int j = 0; j < map_columns; j++)
			{

				rotate_m_mapData[map_columns - 1 - j][i] = m_mapData[i][j];
			}
		}
		ui->x_spinBox->setMaximum(map_rows - 1);
        ui->y_spinBox->setMaximum(map_columns);
        rectangleLabel.setRectangle(mapwidth/(double)map_rows*15,mapheight/(double)map_columns*15);

        BigMapHorizontalslider->setRulerSliderRange(0,map_rows);//设置刻度范围
        BigMapHorizontalslider->setRulerSliderValue(map_rows);//设置当前值

        BigMapVerticalslider->setRulerSliderRange(0,map_columns);//设置刻度范围
        BigMapVerticalslider->setRulerSliderValue(map_columns);//设置当前值
	}

}

void MapTest::on_tableWidget_cellClicked(int row, int column)
{
    if(row==ui->tableWidget->rowCount()-1 && column==1)
    {
        // 设置点击的单元格为可编辑状态
       ui->tableWidget->editItem(ui->tableWidget->item(row, column));

       QTableWidgetItem* item = ui->tableWidget->item(row, column);
          if (item)
          {
              QLineEdit* editor = new QLineEdit(item->text(), ui->tableWidget);

              // 创建一个正则表达式验证器，限制输入为单个非空的ASCII字符
              QRegularExpressionValidator* validator = new QRegularExpressionValidator(QRegularExpression("[ -~]"), editor);
              editor->setValidator(validator);

              editor->setMaxLength(1);  // 设置最大输入长度为 1
              ui->tableWidget->setCellWidget(row, column, editor);
              editor->setFocus();

              /// 重写 focusOutEvent 事件处理函数
              editor->installEventFilter(this);

              // 在编辑器失去焦点时检查输入的有效性
              connect(editor, &QLineEdit::textChanged, [this, editor]() {
                  QString text = editor->text();
                  if (text.isEmpty())
                  {
                      // 输入为空，发出警告或禁止用户离开单元格
                      editor->setFocus();
                      // 例如，您可以显示一个警告对话框，或者强制重新编辑该单元格
                  }
                  else if (text.length() > 1)
                  {
                      // 输入超过一个字符，发出警告或截取第一个字符
                      qDebug() << "Input should be a single character!";
                      // 例如，您可以显示一个警告对话框，并截取第一个字符
                      editor->setText(text.left(1));
                  }
              });
          }

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
                    Paint();
                }
            }
    }



}


// 假设你的字符串匹配函数
bool isMatch(const std::string& str1, const std::string& str2) {
    // 这里是你的字符串匹配逻辑，根据实际情况进行修改
    return str1 == str2;
}

// 定义一个结构体用于表示路径上的每一步
struct PathStep {
    int x;
    int y;
};

// 函数用于计算最短路径并记录路径信息
std::pair<int, std::vector<PathStep>> calculateShortestPath(const QMap<int, QMap<int, std::string>>& m_mapData,
                                                            const std::vector<std::string>& targetChars,
                                                            int startX, int startY,
                                                            int direction) {
    int rows = m_mapData.size();
    int cols = (rows > 0) ? m_mapData.begin()->size() : 0;

    // 初始化 dp 数组和路径记录数组
    std::vector<std::vector<int>> dp(rows, std::vector<int>(cols, INT_MAX));
    std::vector<std::vector<PathStep>> path(rows, std::vector<PathStep>(cols));

    dp[startY][startX] = 0;

    // 定义四个方向的偏移量，分别为右、下、左、上
    const int dx[] = {1, 0, -1, 0};
    const int dy[] = {0, 1, 0, -1};

    // 遍历每个位置
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            // 检查是否匹配目标字符
            for (const std::string& targetChar : targetChars) {
                if (isMatch(m_mapData[y][x], targetChar)) {
                    // 尝试从四个方向更新最短路径
                    for (int k = 0; k < 4; ++k) {
                        int nx = x + dx[k];
                        int ny = y + dy[k];

                        if (nx >= 0 && nx < cols && ny >= 0 && ny < rows) {
                            // 计算距离
                            int distance = dp[y][x] + 1;

                            if (distance < dp[ny][nx]) {
                                dp[ny][nx] = distance;
                                path[ny][nx] = {x, y};
                            }
                        }
                    }
                }
            }
        }
    }

    // 找到最短路径的目标位置
    int shortestPath = INT_MAX;
    int targetX = -1, targetY = -1;

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            if (dp[y][x] < shortestPath) {
                shortestPath = dp[y][x];
                targetX = x;
                targetY = y;
            }
        }
    }

    // 根据 direction 参数选择路径方向
    int currentX = targetX;
    int currentY = targetY;
    std::vector<PathStep> finalPath;

    while (currentX != startX || currentY != startY) {
        finalPath.push_back({currentX, currentY});

        int nextX = path[currentY][currentX].x;
        int nextY = path[currentY][currentX].y;

        qDebug() << "当前X：" << currentX << " 当前Y：" << currentY << " 下一步X：" << nextX << " 下一步Y：" << nextY;

        if (currentX < 0 || currentX >= cols || currentY < 0 || currentY >= rows) {
            qDebug() << "错误：索引超出范围！";
            break;
        }

        currentX = nextX;
        currentY = nextY;
    }

    finalPath.push_back({startX, startY});
    std::reverse(finalPath.begin(), finalPath.end());

    return {shortestPath, finalPath};
}


//struct Node {
//    int x;
//    int y;
//    int distance;
//};

//Node getNextNodePosition(const QMap<int, QMap<int, char>>& m_mapData, char targetChar, int startX, int startY)
//{
//    int maxColumns = m_mapData.size();
//    int maxRows = 0;

//    for (const QMap<int, char>& column : m_mapData) {
//        int rowCount = column.size();
//        if (rowCount > maxRows) {
//            maxRows = rowCount;
//        }
//    }

//    // 初始化标记数组，用于标记节点是否被访问过
//    QVector<QVector<bool>> visited(maxColumns, QVector<bool>(maxRows, false));

//    QQueue<Node> queue;
//    queue.enqueue({startX, startY, 0});
//    visited[startX][startY] = true;

//    while (!queue.isEmpty()) {
//        Node currentNode = queue.dequeue();

//        // 如果当前节点的字符为目标字符，并且不是起始节点，则返回该节点的位置和距离
//        if (m_mapData[currentNode.x][currentNode.y] == targetChar && !(currentNode.x == startX && currentNode.y == startY)) {
//            return currentNode;
//        }

//        // 获取当前节点的邻居节点（上、下、左、右四个方向）
//        int dx[] = {0, 0, -1, 1};
//        int dy[] = {-1, 1, 0, 0};

//        for (int i = 0; i < 4; ++i) {
//            int nextX = currentNode.x + dx[i];
//            int nextY = currentNode.y + dy[i];

//            if (nextX >= 0 && nextX < maxColumns && nextY >= 0 && nextY < maxRows && !visited[nextX][nextY]) {
//                queue.enqueue({nextX, nextY, currentNode.distance + 1});
//                visited[nextX][nextY] = true;
//            }
//        }
//    }

//    // 如果无法找到目标字符，返回一个无效的节点表示失败
//    return {-1, -1, -1};
//}



//struct Node {
//    int x;
//    int y;
//    int distance;
//    std::vector<Node> path; // 记录路径
//};

//Node findOptimalPath(const QMap<int, QMap<int, char>>& m_mapData, char targetChar, int startX, int startY, std::vector<std::vector<bool>>& visited)
//{
//    int maxColumns = m_mapData.size();
//    int maxRows = 0;

//    for (const QMap<int, char>& column : m_mapData) {
//        int rowCount = column.size();
//        if (rowCount > maxRows) {
//            maxRows = rowCount;
//        }
//    }

//    std::queue<Node> queue;
//    queue.push({startX, startY, 0, {}}); // 初始路径为空

//    while (!queue.empty()) {
//        Node currentNode = queue.front();
//        queue.pop();

//        // 检查当前节点的字符是否为目标字符
//        if (m_mapData[currentNode.x][currentNode.y] == targetChar) {
//            // 找到目标字符，返回当前节点
//            // 标记路径上的所有节点为已访问
//            for (const Node& node : currentNode.path) {
//                visited[node.x][node.y] = true;
//            }
//            visited[currentNode.x][currentNode.y] = true;
//            return currentNode;
//        }

//        // 获取当前节点的邻居节点（上、下、左、右四个方向）
//        int dx[] = {0, 0, -1, 1};
//        int dy[] = {-1, 1, 0, 0};

//        for (int i = 0; i < 4; ++i) {
//            int nextX = currentNode.x + dx[i];
//            int nextY = currentNode.y + dy[i];

//            if (nextX >= 0 && nextX < maxColumns && nextY >= 0 && nextY < maxRows && !visited[nextX][nextY]) {
//                std::vector<Node> newPath = currentNode.path; // 复制路径
//                newPath.push_back(currentNode); // 添加当前节点到路径
//                queue.push({nextX, nextY, currentNode.distance + 1, newPath});
//            }
//        }
//    }

//    // 如果无法找到目标字符，返回一个无效的节点表示失败
//    return {-1, -1, -1, {}};
//}

//std::vector<Node> findCompleteOptimalPath(const QMap<int, QMap<int, char>>& m_mapData, const std::vector<char>& targetChars, int startX, int startY)
//{
//    std::vector<Node> completePath;
//    std::vector<char> unvisitedChars = targetChars;

//    int maxColumns = m_mapData.size();
//    int maxRows = 0;

//    for (const QMap<int, char>& column : m_mapData) {
//        int rowCount = column.size();
//        if (rowCount > maxRows) {
//            maxRows = rowCount;
//        }
//    }

//    // 初始化标记数组，用于标记节点是否被访问过
//    std::vector<std::vector<bool>> visited(maxColumns, std::vector<bool>(maxRows, false));

//    while (!unvisitedChars.empty()) {
//        char nearestChar = ' ';
//        Node nearestNode;

//        for (char targetChar : unvisitedChars) {
//            Node result = findOptimalPath(m_mapData, targetChar, startX, startY, visited);
//            if (result.x != -1 && result.y != -1) {
//                if (nearestChar == ' ' || result.distance < nearestNode.distance) {
//                    nearestChar = targetChar;
//                    nearestNode = result;
//                }
//            }
//        }

//        if (nearestChar == ' ') {
//            // 如果无法找到目标字符的最优路径，返回一个空的路径
//            return {};
//        }

//        completePath.insert(completePath.end(), nearestNode.path.begin(), nearestNode.path.end());
//        completePath.push_back(nearestNode);
//        startX = nearestNode.x;
//        startY = nearestNode.y;

//        unvisitedChars.erase(std::remove(unvisitedChars.begin(), unvisitedChars.end(), nearestChar), unvisitedChars.end());
//    }

//    return completePath;
//}

//struct Node {
//    int x;
//    int y;
//};

//struct Point {
//    int x;
//    int y;
//    int steps; // 记录走过的步数
//    std::vector<Node> path; // 记录路径
//};


// 根据给定的方向在矩阵中搜索目标值，并返回找到的位置坐标，如果未找到则返回起始点坐标
std::pair<int, int> traverseSMatrix(const QMap<int, QMap<int, string>>& m_mapDat, const std::vector<string>& targetChars,Direction direction,int startRow, int startCol) {
    const int ROWS = m_mapDat.size();
    const int COLS = m_mapDat.begin()->size();
    bool found = false;

    switch (direction) {
    case LeftToRightTopToBottom:
        for (int i = startRow; i < ROWS; ++i) {
            if (i  % 2 == 0) {
                // 偶数行
                 for (int j = (i == startRow ? startCol : 0); j < COLS; ++j) {
                    for (const auto& target : targetChars)
                    {
                        if (m_mapDat[i][j] == target) {
                            found = true;
                            return {i, j};
                        }
                    }
                }
            } else {
                // 奇数行
                 for (int j = (i == startRow ? startCol : COLS - 1); j >= 0; --j) {
                    for (const auto& target : targetChars)
                    {
                        if (m_mapDat[i][j] == target) {
                            found = true;
                            return {i, j};
                        }
                    }
                }
            }
        }
        break;

    case LeftToRightBottomToTop:
        for (int i = startRow; i >= 0; --i) {
            if (i % 2 == 0) {
                // 偶数行
                 for (int j = (i == startRow ? startCol : 0); j < COLS; ++j){
                    for (const auto& target : targetChars)
                    {
                        if (m_mapDat[i][j] == target) {
                            found = true;
                            return {i, j};
                        }
                    }
                }
            }
            else {
                // 奇数行
                for (int j = (i == startRow ? startCol : COLS - 1); j >= 0; --j) {
                    for (const auto& target : targetChars)
                    {
                        if (m_mapDat[i][j] == target) {
                            found = true;
                            return {i, j};
                        }
                    }
                }
            }
        }
        break;
    case RightToLeftTopToBottom:
        for (int i = startRow; i < ROWS; ++i) {
            if (i % 2 == 0) {
                // 偶数行
                for (int j = (i == startRow ? startCol : COLS - 1); j >= 0; --j) {
                    for (const auto& target : targetChars)
                    {
                        if (m_mapDat[i][j] == target) {
                            found = true;
                            return {i, j};
                        }
                    }
                }
            }
            else {
                // 奇数行
                for (int j = (i == startRow ? startCol : 0); j < COLS; ++j) {
                    for (const auto& target : targetChars)
                    {
                        if (m_mapDat[i][j] == target) {
                            found = true;
                            return {i, j};
                        }
                    }
                }
            }
        }
        break;

    case RightToLeftBottomToTop:
        for (int i = startRow; i >= 0; --i) {
            if (i % 2 == 0) {
                // 偶数行
                for (int j = (i == startRow ? startCol : COLS - 1); j >= 0; --j)  {
                    for (const auto& target : targetChars)
                    {
                        if (m_mapDat[i][j] == target) {
                            found = true;
                            return {i, j};
                        }
                    }
                }
            }
            else {
                // 奇数行
                for (int j = (i == startRow ? startCol : 0); j < COLS; ++j) {
                    for (const auto& target : targetChars)
                    {
                        if (m_mapDat[i][j] == target) {
                            found = true;
                            return {i, j};
                        }
                    }
                }
            }
        }
        break;

    case TopToBottomLeftToRight:
        for (int j = startCol; j < COLS; ++j) {
            if (j % 2 == 0){
                // 偶数行
                for (int i = (j == startCol ? startRow : 0); i < ROWS; ++i) {
                    for (const auto& target : targetChars)
                    {
                        if (m_mapDat[i][j] == target) {
                            found = true;
                            return {i, j};
                        }
                    }
                }
            }
            else {
                // 奇数行
               for (int i = (j == startCol ? startRow : ROWS - 1); i >= 0; --i) {
                    for (const auto& target : targetChars)
                    {
                        if (m_mapDat[i][j] == target) {
                            found = true;
                            return {i, j};
                        }
                    }
                }
            }
        }
        break;

    case TopToBottomRightToLeft:
        for (int j = startCol; j >= 0; --j) {
            if ( j % 2 == 0) {
                // 偶数行
               for (int i = (j == startCol ? startRow : 0); i < ROWS; ++i) {
                    for (const auto& target : targetChars)
                    {
                        if (m_mapDat[i][j] == target) {
                            found = true;
                            return {i, j};
                        }
                    }
                }
            }
            else {
                // 奇数行
                for (int i = (j == startCol ? startRow : ROWS - 1); i >= 0; --i) {
                    for (const auto& target : targetChars)
                    {
                        if (m_mapDat[i][j] == target) {
                            found = true;
                            return {i, j};
                        }
                    }
                }
            }
        }
        break;

    case BottomToTopLeftToRight:
        for (int j = startCol; j < COLS; ++j) {
            if (j % 2 == 0)  {
                // 偶数行
                for (int i = (j == startCol ? startRow : ROWS - 1); i >= 0; --i) {
                    for (const auto& target : targetChars)
                    {
                        if (m_mapDat[i][j] == target) {
                            found = true;
                            return {i, j};
                        }
                    }
                }
            }
            else {
                // 奇数行
                for (int i = (j == startCol ? startRow : 0); i < ROWS; ++i)  {
                    for (const auto& target : targetChars)
                    {
                        if (m_mapDat[i][j] == target) {
                            found = true;
                            return {i, j};
                        }
                    }
                }
            }
        }
        break;

    case BottomToTopRightToLeft:
        for (int j = startCol; j >= 0; --j) {
            if (j % 2 == 0) {
                // 偶数行
                for (int i = (j == startCol ? startRow : ROWS - 1); i >= 0; --i) {
                    for (const auto& target : targetChars)
                    {
                        if (m_mapDat[i][j] == target) {
                            found = true;
                            return {i, j};
                        }
                    }
                }
            }
            else {
                // 奇数行
                for (int i = (j == startCol ? startRow : 0); i < ROWS; ++i) {
                    for (const auto& target : targetChars)
                    {
                        if (m_mapDat[i][j] == target) {
                            found = true;
                            return {i, j};
                        }
                    }
                }
            }
        }
        break;

    default:
        // 返回起始点坐标
        return {0, 0};
    }

    // 如果未找到目标，返回起始点坐标
    if (!found) {
        return {0, 0};
    }
}

//void findPaths(const QMap<int, QMap<int, string>>& m_mapData, const std::vector<string>& targetChars, int startX, int startY, std::vector<Point>& allPaths, std::vector<string>& unvisitedChars, std::vector<std::vector<bool>>& visited) {
//    int m = m_mapData.size();
//    int n = m_mapData.empty() ? 0 : m_mapData.values().first().size();

//    std::vector<Point> stack;
//    stack.push_back({startX, startY, 0, {}}); // 初始路径为空

//    int dx[] = {0, 0, -1, 1};
//    int dy[] = {-1, 1, 0, 0};

//    while (!stack.empty()) {
//        Point p = stack.back();
//        stack.pop_back();

//        if (std::find(targetChars.begin(), targetChars.end(), m_mapData[p.x][p.y]) != targetChars.end()) {
//            unvisitedChars.erase(std::remove(unvisitedChars.begin(), unvisitedChars.end(), m_mapData[p.x][p.y]), unvisitedChars.end()); // 到达目标字符，将其从未访问字符集合中移除
//            if (unvisitedChars.empty()) {
//                allPaths.push_back(p); // 所有目标字符均已访问，将路径添加到结果集合
//                continue; // 继续搜索其他路径
//            }
//        }

//        for (int i = 0; i < 4; ++i) {
//            int nx = p.x + dx[i];
//            int ny = p.y + dy[i];

//            if (nx >= 0 && nx < m && ny >= 0 && ny < n && !visited[nx][ny]) {
//                std::vector<Node> newPath = p.path; // 复制路径
//                newPath.push_back({nx, ny}); // 添加当前节点到路径
//                stack.push_back({nx, ny, p.steps + 1, newPath});
//                visited[nx][ny] = true;
//            }
//        }
//    }
//}



//// 定义九宫格中的方向
//int dx[] = {-1, 0, 1, 0};
//int dy[] = {0, 1, 0, -1};

// 定义九宫格中的点
typedef pair<int, int> Point;

//// 判断点是否在九宫格内
//bool isValid(int x, int y, const QMap<int, QMap<int, string>>& m_mapData) {
//    return m_mapData.contains(x) && m_mapData[x].contains(y);
//}

//// 使用BFS查找从起点到终点，并经过的障碍物数量最少的路径
//vector<Point> bfs(QMap<int, QMap<int, string>>& m_mapData,  Point& start,  Point& end,  vector<string>& targetChars) {
//    // 创建队列，并将起始节点添加到路径中
//    queue<pair<vector<Point>, int>> q;
//    q.push(make_pair(vector<Point>{start}, 0));

//    // 记录已访问的位置
//    QMap<int, QMap<int, bool>> visited;
//    visited[start.first][start.second] = true;

//    // 转换目标字符集为无序集合，以提高查找效率
//    std::unordered_set<string> targetCharsSet(targetChars.begin(), targetChars.end());

//    // 记录路径中的障碍物数量最少的路径
//    vector<Point> minObstaclesPath;

//    // BFS
//    while (!q.empty()) {
//        auto& front = q.front();
//        auto& path = front.first;
//        int numObstacles = front.second;
//        q.pop();

//        // 检查当前位置是否是终点
//        if (!path.empty() && path.back() == end) {
//            if (minObstaclesPath.empty() || numObstacles < minObstaclesPath.size() - 1) {
//                minObstaclesPath = path;
//            }
//            continue;
//        }

//        // 遍历当前位置的四个方向
//        for (int i = 0; i < 4; ++i) {
//            int nx = (path.empty() ? start.first : path.back().first) + dx[i];
//            int ny = (path.empty() ? start.second : path.back().second) + dy[i];

//            // 如果下一个位置有效且未访问过
//            if (isValid(nx, ny, m_mapData) && !visited[nx][ny]) {
//                // 标记下一个位置已访问
//                visited[nx][ny] = true;

//                // 如果下一个位置是障碍物，增加障碍物数量
//                vector<Point> nextPath = path;
//                nextPath.push_back(make_pair(nx, ny));
//                if (targetCharsSet.count(m_mapData[nx][ny])) {
//                    q.push(make_pair(nextPath, numObstacles + 1));
//                } else {
//                    q.push(make_pair(nextPath, numObstacles));
//                }
//            }
//        }
//    }

//    return minObstaclesPath;
//}


struct Node {
    int x, y, distance;
};

vector<Node> bfs(const QMap<int, QMap<int, string>>& m_mapData, const Point& start, const Point& end, const vector<string>& targetChars) {
    int maxColumns = m_mapData.size();
    int maxRows = 0;

    for (const auto& column : m_mapData) {
        int rowCount = column.size();
        if (rowCount > maxRows) {
            maxRows = rowCount;
        }
    }

    // 初始化标记数组，用于标记节点是否被访问过
    QVector<QVector<bool>> visited(maxColumns, QVector<bool>(maxRows, false));

    queue<Node> q;
    q.push({start.first, start.second, 0});
    visited[start.first][start.second] = true;

    vector<Node> minObstaclesPath;

    while (!q.empty()) {
        Node currentNode = q.front();
        q.pop();

        // 如果当前节点是终点，更新最小障碍物路径
        if (currentNode.x == end.first && currentNode.y == end.second) {
            if (minObstaclesPath.empty() || currentNode.distance < minObstaclesPath.size() - 1) {
                minObstaclesPath.push_back(currentNode);
            }
            continue;
        }

        // 获取当前节点的邻居节点（上、下、左、右四个方向）
        int dx[] = {0, 0, -1, 1};
        int dy[] = {-1, 1, 0, 0};

        for (int i = 0; i < 4; ++i) {
            int nextX = currentNode.x + dx[i];
            int nextY = currentNode.y + dy[i];

            if (nextX >= 0 && nextX < maxColumns && nextY >= 0 && nextY < maxRows && !visited[nextX][nextY]) {
                // 如果下一个位置是目标字符，增加障碍物数量
                string nextChar = m_mapData[nextX][nextY];
                if (find(targetChars.begin(), targetChars.end(), nextChar) != targetChars.end()) {
                    q.push({nextX, nextY, currentNode.distance + 1});
                } else {
                    q.push({nextX, nextY, currentNode.distance});
                }
                visited[nextX][nextY] = true;
            }
        }
    }

    return minObstaclesPath;
}


//开始
void MapTest::on_start_pushButton_clicked()
{

    RecordCurrent_x+=BigMapX;
    RecordCurrent_y+=BigMapY;
    BigMapX=0;
    BigMapY=0;

    if(rodegrees == 0 || rodegrees == 180)
    {
      rectangleLabel.setRectangle(mapwidth/(double)map_columns*15,mapheight/(double)map_rows*15);
      rectangleLabel.UpdateRectangle(QRectF((RecordCurrent_x-7)*mapwidth/(double)map_columns,(RecordCurrent_y-7)*mapheight/(double)map_rows,mapheight/(double)map_columns*15,mapheight/(double)map_rows*15));
    }
    else if(rodegrees == 90 || rodegrees == 270)
    {
       rectangleLabel.setRectangle(mapwidth/(double)map_rows*15,mapheight/(double)map_columns*15);
       rectangleLabel.UpdateRectangle(QRectF((RecordCurrent_x -7)*mapwidth/(double)map_rows,(RecordCurrent_y -7)*mapheight/(double)map_columns,mapwidth/(double)map_rows*15,mapheight/(double)map_columns*15));
    }
    m_scaleFactor=1;
    ui->main_windows_label->setPixmap(QPixmap::fromImage(Image));

    on_sure_pushButton_clicked();
    m_pTimer->start(1);

//    std::vector<char> targetChars = {'1'};
//    int startX = ui->x_spinBox->text().toInt();
//    int startY = ui->y_spinBox->text().toInt();

//    std::vector<Node> result = findCompleteOptimalPath(m_mapData, targetChars, startY, startX);

//    if (!result.empty()) {
//        std::cout << "完整路径的长度为：" << result.back().distance << std::endl;
//        std::cout << "完整路径为：";
//        for (const Node& node : result) {
//            std::cout << "(" << node.x << ", " << node.y << ") -> ";
//        }
//        std::cout << std::endl;
//    } else {
//        std::cout << "无法找到完整路径" << std::endl;
//    }


//        std::vector<string> targetChars = {"1"}; // 目标字符
//        int startX = ui->x_spinBox->text().toInt(); // 起点的x坐标
//        int startY = ui->y_spinBox->text().toInt(); // 起点的y坐标

//            int m = m_mapData.size();
//            int n = m_mapData.empty() ? 0 : m_mapData.values().first().size();
//            std::vector<std::vector<bool>> visited(m, std::vector<bool>(n, false));

//            std::vector<Point> allPaths;
//            std::vector<string> unvisitedChars(targetChars.begin(), targetChars.end()); // 记录未访问过的目标字符

//            visited[startX][startY] = true;

//            findPaths(m_mapData, targetChars, startX, startY, allPaths, unvisitedChars, visited);

//            if (!allPaths.empty()) {
//                std::cout << "All shortest paths to target characters:" << std::endl;
//                for (const Point& path : allPaths) {
//                    std::cout << "Path: ";
//                    for (const Node& node : path.path) {
//                        std::cout << "(" << node.x << ", " << node.y << ") -> ";
//                    }
//                    std::cout << "(" << path.x << ", " << path.y << ")" << std::endl;
//                }
//            } else {
//                std::cout << "No paths found to all target characters." << std::endl;
//            }

            // 目标字符向量
                //std::vector<std::string> targetChars = {"1", "2", "3"};

                // 起点坐标
                //int startX = ui->x_spinBox->text().toInt(); // 起点的x坐标
                //int startY = ui->y_spinBox->text().toInt(); // 起点的y坐标
                // 方向参数，1表示从上到下，2表示从下到上，3表示从左到右，4表示从右到左
               //int direction = 1;

               // 计算最短路径和记录路径信息
               //auto result = calculateShortestPath(m_mapData, targetChars, startX, startY, direction);


                // 调用traverseSMatrix函数进行搜索
                 // std::pair<int, int> result = traverseSMatrix(m_mapData,targetChars,LeftToRightTopToBottom,startX,startY);

//               // 打印结果
               // std::cout << "最短路径长度为: " << result.first <<result.second<< std::endl;


               //std::cout << std::endl;

}

//停止
void MapTest::on_stop_pushButton_clicked()
{

    m_pTimer->stop();
    QPixmap pixmap = *ui->main_windows_label->pixmap();
    pixmap.save("./chip.png");
    Image = pixmap.toImage();
}

void MapTest::handleTimeout()
{
    int startX =  ui->x_spinBox->text().toInt(); // Starting X position
    int startY =  ui->y_spinBox->text().toInt(); // Starting Y position

    // 目标字符向量
    std::vector<std::string> targetChars;


    for (int row = 0; row < ui->tableWidget->rowCount(); ++row)
    {
       if(getCheckBox(ui->tableWidget,row,0)->checkState())
       {

            QString data = ui->tableWidget->model()->index(row, 1).data().toString();
            targetChars.push_back(data.toStdString());
       }
    }

    // 根据索引获取对应的枚举值
     Direction selectedDirection = static_cast<Direction>(ui->upDown_comboBox->currentIndex());

    std::pair<int, int> result = traverseSMatrix(m_mapData,targetChars,selectedDirection,startY,startX);

    //Node nextNodePosition = getNextNodePosition(m_mapData, '1', startY, startX);

    if (result.first != 0 || result.second != 0)
    {
        qDebug() << "下一个节点位置：" << result.first<< ", " << result.second;
    }
    else
    {
        m_pTimer->stop();
        qDebug() << "没有找到目标字符的路径。";
    }

    // 定义起点和终点
    //Point start =m ake_pair(startX, startY);

    //Point start = make_pair(startY, startX);

    // 求从起点到终点，经过的障碍物数量最少的路径
    //vector<Node> minObstaclesPath = bfs(m_mapData, start, result, targetChars);


    // 输出结果
    //if (minObstaclesPath.empty()) {
     //   qDebug() << "无法到达终点";
    //} else {
    //    qDebug() << "经过障碍物数量最少的路径：";
     //   for (const auto& point : minObstaclesPath) {
    //        qDebug() << point.x << ", " << point.y;
     //   }
    //}

    ui->x_spinBox->setValue(result.second);
    ui->y_spinBox->setValue(result.first);
    on_sure_pushButton_clicked();

}
