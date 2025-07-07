#pragma once

#include <QWidget>
 
#include "ui_playvideo.h"
#include "demux_task.h"
#include "decode_task.h"
#include "video_view.h"

class PPlayVideo : public QWidget
{
	Q_OBJECT

public:
	PPlayVideo(QWidget *parent = nullptr);
	~PPlayVideo();

	bool Open(const char* url);
	void timerEvent(QTimerEvent* event) override;
	void Close();
	void closeEvent(QCloseEvent* event)override;
private:
	Ui::PlayVideoClass ui;
	DemuxTask demux_;
	DecodeTask decodec_;
	Video_View* view_ = nullptr;
};

