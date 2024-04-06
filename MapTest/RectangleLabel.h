#ifndef RECTANGLELABEL_H
#define RECTANGLELABEL_H

#include <QLabel>
#include <QPainter>
#include <QMouseEvent>

class RectangleLabel : public QLabel {
    Q_OBJECT

public:
    RectangleLabel(QWidget *parent = nullptr);

    void setRectangle(double width,double height);
    void UpdateRectangle(const QRectF rec);
protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    QRectF rectangle;

};

#endif // RECTANGLELABEL_H
