#include "calendar.h"
#include <QPainter>

Calendar::Calendar(QWidget* p):QCalendarWidget(p)
{

}

void Calendar::paintCell(QPainter* painter, const QRect& rect, QDate date) const
{
	//有视频的日期特殊显示
	if (mdate_.find(date)==mdate_.end())//没有视频
	{
		QCalendarWidget::paintCell(painter, rect, date);
		return;
	}

	auto font = painter->font();
	font.setPixelSize(30);
	if (date == selectedDate())
	{
		painter->setBrush(QColor(118, 178, 244));	//刷子颜色
		painter->drawRect(rect);					//绘制背景
	}
	painter->setFont(font);						//设置字体和颜色
	painter->setPen(QColor(255, 0, 0));			//字体颜色
	painter->drawText(rect,Qt::AlignCenter,QString::number(date.day()));
	
}
