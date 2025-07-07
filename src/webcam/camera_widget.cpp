#include "camera_widget.h"
#include "demux_task.h"
#include "decode_task.h"
#include "video_view.h"
#include "camconfig.h"

#include <QStyleOption>
#include <QPainter>
#include <QStyle>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QListWidget>

CameraWidget::CameraWidget(QWidget* parent):QWidget(parent)
{
	//接收拖拽
	this->setAcceptDrops(true);
	
}

void CameraWidget::dragEnterEvent(QDragEnterEvent* event)
{
	//接收拖拽进入
	event->acceptProposedAction();
}

void CameraWidget::dropEvent(QDropEvent* event)
{
	//拿到url
	auto wid = (QListWidget*)event->source();
	auto cam = CamConfig::Instance()->GetCam(wid->currentRow());
	Open(cam.sub_url);
}

void CameraWidget::paintEvent(QPaintEvent* event)
{
	//渲染样式表
	QStyleOption opt;
	opt.initFrom(this);
	QPainter p(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

bool CameraWidget::Open(const char* url)
{
	if (demux_)demux_->Stop();
	if (decode_)decode_->Stop();
	//打开解封装
	demux_ = new DemuxTask();
	if (!demux_->Open(url))return false;
	//打开视频解码器线程
	decode_ = new DecodeTask();
	auto para = demux_->CopyVideoPara();
	if (!decode_->Open(para->para)) return false;
	//设定解码线程接收解封装数据
	demux_->set_next(decode_);
	//初始化渲染参数
	view_ = Video_View::Create();
	view_->set_win_id((void*)winId());
	view_->Init(para->para);
	//启动解封装和解码线程
	demux_->Start();
	decode_->Start();
	return true;
}

void CameraWidget::Draw()
{
	if (!decode_ || !demux_ || !view_)return;
	auto f = decode_->GetFrame();
	view_->DrawFrame(f);
	FreeFrame(&f);
}

CameraWidget::~CameraWidget()
{
	if (demux_)
	{
		demux_->Stop();
		delete demux_;
		demux_ = nullptr;
	}
	if (decode_)
	{
		decode_->Stop();
		delete decode_;
		decode_ = nullptr;
	}
	if (view_)
	{
		view_->Close();
		delete view_;
		view_ = nullptr;
	}
}
