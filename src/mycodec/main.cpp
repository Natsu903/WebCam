#include <iostream>
#include <thread>
#include <chrono>
#include "tools.h"
#include "demux_task.h"
#include "decode_task.h"
#include "video_view.h"
#include "mux_task.h"

#define CAM1 "rtsp://admin:GZH&password@192.168.31.235/cam/realmonitor?channel=1&subtype=0"

int main(int argc, char* argv[])
{
	DemuxTask demux_task;
	for (;;)
	{
		if(demux_task.Open(CAM1))break;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		continue;
	}
	auto vpara = demux_task.CopyVideoPara();
	AVCodecParameters* video_para = nullptr;
	AVCodecParameters* audio_para = nullptr;
	AVRational* video_time_base = nullptr;
	AVRational* audio_time_base = nullptr;
	if (vpara)
	{
		video_para = vpara->para;
		video_time_base = vpara->time_base;
	}
	auto apara = demux_task.CopyAudioPara();
	if (apara)
	{
		audio_para = apara->para;
		audio_time_base = apara->time_base;
	}
	MuxTask mux_task;
	if (!mux_task.Open("rtsp_out1.mp4", video_para, video_time_base, audio_para, audio_time_base))
	{
		LOGERROR("mux_task.Open failed");
		return -1;
	}
	demux_task.set_next(&mux_task);
	demux_task.Start();
	mux_task.Start();
	MSleep(5000);
	mux_task.Stop();

	if (!mux_task.Open("rtsp_out2.mp4", video_para, video_time_base, audio_para, audio_time_base))
	{
		LOGERROR("mux_task.Open failed");
		return -1;
	}
	mux_task.Start();
	MSleep(5000);
	mux_task.Stop();

	getchar();
	return 0;
}
