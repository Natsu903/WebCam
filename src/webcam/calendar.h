#pragma once
#include <QCalendarWidget>
#include <set>

class Calendar :public QCalendarWidget
{
public:
	Calendar(QWidget* p);
	void paintCell(QPainter* painter, const QRect& rect, QDate date) const override;
	inline void AddDate(QDate d) { mdate_.insert(d); }
	inline void ClearDate() { mdate_.clear(); }
private:
	std::set<QDate> mdate_;//存放有视频的日期
};

