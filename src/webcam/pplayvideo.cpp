#include "PPlayVideo.h"

bool PPlayVideo::Open(const char* url)
{
	Close();
	if (!demux_.Open(url)) return false;
	auto vp = demux_.CopyVideoPara();
	if (!vp) return false;
	if (!decodec_.Open(vp->para)) return false;
	demux_.set_next(&decodec_);
	if (!view_) view_ = Video_View::Create();
	view_->set_win_id((void*)winId());
	if (!view_->Init(vp->para)) return false;
	demux_.set_syn_type(SYN_VIDEO);
	demux_.Start();
	decodec_.Start();
	return true;
}

void PPlayVideo::timerEvent(QTimerEvent* event)
{
	if (!view_)return;
	auto f = decodec_.GetFrame();
	if (!f)return;
	view_->DrawFrame(f);
	FreeFrame(&f);
}

void PPlayVideo::Close()
{
	demux_.Stop();
	decodec_.Stop();
	if (view_)
	{
		view_->Close();
		delete view_;
		view_ = nullptr;
	}
}

void PPlayVideo::closeEvent(QCloseEvent* event)
{
	Close();
}

PPlayVideo::PPlayVideo(QWidget *parent): QWidget(parent)
{
	ui.setupUi(this);
	startTimer(10);
}

PPlayVideo::~PPlayVideo()
{
	Close();
}

