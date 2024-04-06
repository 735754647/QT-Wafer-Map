
#ifndef MYLABEL_H
#define MYLABEL_H

#include<QLabel>

#include<QPoint>
#include<QMouseEvent>
#include<QPainter>
#include<QDebug>

#include<QString>


using namespace std;


class MyLabel : public QLabel
{
	Q_OBJECT
public:

	MyLabel(QWidget *parent = 0);


	void paintEvent(QPaintEvent * event);

    void UpdateRectangle();

private:

	virtual void mousePressEvent(QMouseEvent*) override;

};

#endif // MYLABEL_H
