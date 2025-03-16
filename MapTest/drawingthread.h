#ifndef DRAWINGTHREAD_H
#define DRAWINGTHREAD_H


#include <QMainWindow>
#include <QTableWidget>
#include <string>
#include <QMutex>
#include <QThread>
#include <QPainter>
#include<unordered_map>
class DrawingThread : public QThread
{
    Q_OBJECT

public:
    explicit DrawingThread(QTableWidget* tableWidget,QMap<int, QMap<int, std::string>> mapData, double width, double height,int columns, int rows, QObject *parent = nullptr);

    void UpdateCurrentlyParam(QTableWidget *tableWidget, QMap<int, QMap<int, std::string> > mapData, double width, double height, int columns, int rows);
protected:
    void run() override;

signals:
    void finished();

private:
    QMap<int, QMap<int, std::string>> m_mapData;
    QMap<int, QMap<int, std::string>> tempmapData;
    QTableWidget* m_tableWidget;
    double m_width;
    double m_height;
    int m_columns;
    int m_rows;
    QImage m_image;
    QMutex m_mutex;
};
#endif // DRAWINGTHREAD_H
