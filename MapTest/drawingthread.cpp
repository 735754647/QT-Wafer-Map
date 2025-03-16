#include "drawingthread.h"

DrawingThread::DrawingThread(QTableWidget* tableWidget,QMap<int, QMap<int, std::string>> mapData, double width, double height, int columns, int rows, QObject *parent)
    : QThread(parent)
{
    m_tableWidget=tableWidget;
    m_mapData=mapData;
    m_width=width;
    m_height=height;
    m_columns=columns;
    m_rows=rows;


}


void DrawingThread::UpdateCurrentlyParam(QTableWidget* tableWidget,QMap<int, QMap<int, std::string>> mapData, double width, double height, int columns, int rows)
{
    m_tableWidget=tableWidget;
    m_mapData=mapData;
    m_width=width;
    m_height=height;
    m_columns=columns;
    m_rows=rows;
}



void DrawingThread::run()
{
    // Perform your drawing operations here
    // This is just a placeholder code

    // 缓存数据到变量
    QAbstractItemModel* model = m_tableWidget->model();
    int rowCount = model->rowCount();

    m_mutex.lock();

    // 使用无序映射进行数据查找
    std::unordered_map<std::string, QColor> colorMap;
    for (int n = 0; n < rowCount; n++)
    {
        QString data = model->index(n, 1).data().toString();
        std::string ch = data.toStdString();
        QColor color = QColor(model->index(n, 4).data().toString());
        colorMap[ch] = color;
    }

    m_image = QImage(m_width * m_columns, m_height * m_rows, QImage::Format_RGB32);
    QPainter painter(&m_image);

    for (int i = 0; i < m_rows; i++)
    {
        for (int j = 0; j < m_columns; j++)
        {
            std::string ch = m_mapData[i][j];

            //if(tempmapData.size()==0||tempmapData[i][j]!=m_mapData[i][j])
            {
                // 从无序映射中查找颜色
                auto iter = colorMap.find(ch);
                if (iter != colorMap.end())
                {
                    painter.fillRect(QRectF(j * m_width, i * m_height, m_width, m_height), iter->second);
                }
                else
                {
                    painter.fillRect(QRectF(j * m_width, i * m_height, m_width, m_height), Qt::white);
                }
            }
        }
    }
    //tempmapData=m_mapData;
    m_image.save("./chip.png", "PNG");	//输出图片
    m_mutex.unlock();

    // Signal that the image is ready
    emit finished();
}
