#include "PPlayVideo.h"
#include <QDebug>

bool PPlayVideo::Open(const char* url)
{
	if (!player.Open(url, (void*)ui.video->winId())) return false;
	player.Start();
	player.Pause(false);//默认播放状态
	startTimer(10);
	return true;
}

void PPlayVideo::timerEvent(QTimerEvent* event)
{
	if (player.is_pause())
	{
		ui.playbtn->setIcon(QIcon(":/playvideo/img/play_surface.png"));
	}
	else
	{
		ui.playbtn->setIcon(QIcon(":/playvideo/img/pause_surface.png"));
	}
	if (player.is_pause())return;
	player.Update();
	auto pos = player.pos_ms();
	auto total = player.total_ms();
	ui.pos_slider->setMaximum(total);
	ui.pos_slider->setMinimum(0);
	ui.pos_slider->setValue(pos);
}

void PPlayVideo::Close()
{
	player.Stop();
}

void PPlayVideo::closeEvent(QCloseEvent* event)
{
	Close();
}

void PPlayVideo::PlayPos()
{
	player.Seek(ui.pos_slider->value());
	//如果拖动进度条前没暂停,恢复播放
	if (!paused_before_)
	{
		player.Pause(false);
	}
}

void PPlayVideo::Pause()
{
	player.Pause(!player.is_pause());
}

void PPlayVideo::Move()
{
	if(!paused_before_)
		player.Pause(true);
}

void PPlayVideo::Pressed()
{
	//如果拖动进度条前就暂停了
	if (player.is_pause())
	{
		//记录暂停状态
		paused_before_ = true;
		qDebug() << "paused_before_ = true;";
	}
	else
	{
		paused_before_ = false;
		qDebug() << "paused_before_ = false;";
	}
}

void PPlayVideo::SetSpeed()
{
	float speed = 1;
	int s = ui.speed_slider->value();
	if (s <= 10)
	{
		speed = (float)s / (float)10;
	}
	else
	{
		speed = s - 9;
	}
	ui.speed_now->setText(QString::number(speed).append("x"));
	qDebug() << "调整播放速度到:" << speed << "x";
	player.SetSpeed(speed);
}

PPlayVideo::PPlayVideo(QDialog *parent): QDialog(parent)
{
	ui.setupUi(this);
}

PPlayVideo::~PPlayVideo()
{
	Close();
}

