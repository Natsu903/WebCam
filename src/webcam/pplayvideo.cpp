#include "PPlayVideo.h"

bool PPlayVideo::Open(const char* url)
{
	if (!player.Open(url, (void*)winId())) return false;
	player.Start();
	startTimer(10);
	return true;
}

void PPlayVideo::timerEvent(QTimerEvent* event)
{
	player.Update();
}

void PPlayVideo::Close()
{
	player.Stop();
}

void PPlayVideo::closeEvent(QCloseEvent* event)
{
	Close();
}

PPlayVideo::PPlayVideo(QDialog *parent): QDialog(parent)
{
	ui.setupUi(this);
}

PPlayVideo::~PPlayVideo()
{
	Close();
}

