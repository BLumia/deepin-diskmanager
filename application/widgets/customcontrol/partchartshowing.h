/*
* Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
*
* Author:     zhangkai <zhangkai@uniontech.com>
* Maintainer: zhangkai <zhangkai@uniontech.com>
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef PARTCHARTSHOWING_H
#define PARTCHARTSHOWING_H
#include <DToolTip>
#include <DWidget>
#include <DApplicationHelper>
#include <QWidget>
#include <QRect>
#include <QPainterPath>
#include <QPainter>
#include <DPalette>
#include <QBrush>
#include <QPointF>
#include <QLine>
#include <QMouseEvent>
#include <QTimer>

DWIDGET_USE_NAMESPACE

class PartChartShowing : public QWidget
{
    Q_OBJECT
public:
    explicit PartChartShowing(QWidget *parent = nullptr);
    //    PartChartShowing(int used, int trueused, int noused, QWidget *parent = nullptr);
    void transInfos(const double &totals, const QVector<double> sizeInfo);
    void transFlag(int mflag, int value = 0);
signals:
    void sendFlag(int flag, int num, int posX);
    void sendMoveFlag(int moveflag, int num, int posx);
    void judgeLastPartition();
public slots:
    void showTipTimerSlot();
protected:
    virtual void paintEvent(QPaintEvent *event) override;
    void addPaint(QPainter *painter);
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
private:
    int flag = 1;
    QString m_totalsize;
    double total = 0.00;
    QVector<double> partsize;
    QPoint curPoint;
    QRect getCurRect;
    QVector<QColor> basecolor;
    QVector<QPainterPath> allpath;
    QTimer *showTipTimer;
    int number = -1;
    int number2 = -1;
    double sums = 0.00;
    int i = 0;
//    int count = 0;
    double sumvalue = 0.0000001;
    int hover = 0;
    int mx = 0;
    int showTipFlag = 0;
    bool mShowFlag = false;
    int mnum = 0;
};

#endif // PARTCHARTSHOWING_H
