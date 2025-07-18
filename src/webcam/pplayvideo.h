#pragma once

#include <QDialog>
 
#include "ui_playvideo.h"
#include "demux_task.h"
#include "decode_task.h"
#include "video_view.h"
#include "player.h"

class PPlayVideo : public QDialog
{
	Q_OBJECT

public:
	PPlayVideo(QDialog *parent = nullptr);
	~PPlayVideo();

	bool Open(const char* url);
	void timerEvent(QTimerEvent* event) override;
	void Close();
	void closeEvent(QCloseEvent* event)override;
public slots:
	void SetSpeed();
private:
	Ui::PlayVideoClass ui;
	Player player;
};

