#pragma once
#include "tools.h"
#include "demux_task.h"
#include "decode_task.h"
#include "video_view.h"
class WEBCAM_API Player :public BaseThread
{
public:
	//回调接收音视频包
	void Do(AVPacket* pkt)override;
	//打开音视频处理渲染播放
	bool Open(const char* url, void* winid);
	void Stop();
	//主线程处理同步
	void Main();
	//开启解封装、音视频解码和处理同步
	void Start();
	//渲染视频播放音频
	void Update();
protected:
	DemuxTask demux_;			//解封装
	DecodeTask audio_decode_;	//音频解码
	DecodeTask video_decode_;	//视频解码
	Video_View* view_ = nullptr;//视频渲染
};

