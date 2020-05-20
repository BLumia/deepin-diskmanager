#ifndef DMFRAMEWIDGET_H
#define DMFRAMEWIDGET_H

#include <QWidget>
#include <DFrame>
#include <QPainter>
#include <DPalette>
#include <DApplicationHelper>
#include <DFontSizeManager>
#include "dmtreeviewdelegate.h"
#include "partitioninfo.h"
#include <QTextOption>
DWIDGET_USE_NAMESPACE


class DmFrameWidget : public DFrame
{
    Q_OBJECT
public:
    explicit DmFrameWidget(DiskInfoData data, QWidget *parent = nullptr);

signals:

public:
    void setFrameData(const PartitionInfo &data);

public slots:
    void slothandleChangeTheme();
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    DiskInfoData m_infodata;
    DPalette m_parentPb;
};

#endif // DMFRAMEWIDGET_H
