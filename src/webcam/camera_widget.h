#pragma once
#include <QWidget>

class DecodeTask;
class DemuxTask;
class Video_View;

class CameraWidget :public QWidget
{
	Q_OBJECT

public:
	CameraWidget(QWidget* parent=nullptr);

	//拖拽进入
	void dragEnterEvent(QDragEnterEvent* event) override;
	//拖拽松开
	void dropEvent(QDropEvent* event)override;
	//渲染
	void paintEvent(QPaintEvent* event)override;
	//解封装解码rtsp
	bool Open(const char* url);
	//渲染视频
	void Draw();

	~CameraWidget();
private:
	DecodeTask* decode_ = nullptr;
	DemuxTask* demux_ = nullptr;
	Video_View* view_ = nullptr;
};

