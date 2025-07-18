#include "PPlayVideo.h"

bool PPlayVideo::Open(const char* url)
{
	if (!player.Open(url, (void*)ui.video->winId())) return false;
	player.Start();
	startTimer(10);
	return true;
}

void PPlayVideo::timerEvent(QTimerEvent* event)
{
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

void PPlayVideo::SetSpeed()
{
	float speed = 1;
	int s = ui.seppd_slider->value();
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

