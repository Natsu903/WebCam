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
	//控制播放速度
	void SetSpeed();
	//进度条松开控制播放进度
	void PlayPos();
	//暂停、播放
	void Pause();
	//进度条拖动
	void Move();
	//用于判断进度条按下前是否为暂停状态
	void Pressed();
private:
	//用于判断拖动进度条之前是否暂停
	bool paused_before_ = false;
	Ui::PlayVideoClass ui;
	Player player;
};

