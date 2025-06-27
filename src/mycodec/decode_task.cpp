#include "decode_task.h"
#include <iostream>
#include <thread>
#include <chrono>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

void DecodeTask::Main()
{
	{
		std::unique_lock<std::mutex>lock(mux_);
		if (!frame_)
		{
			frame_ = av_frame_alloc();
		}
	}
	
	while (!is_exit_)
	{
		auto pkt = pkt_list_.Pop();
		if (!pkt)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}
		//发送到解码线程
		bool re = decode_.Send(pkt);
		av_packet_free(&pkt);
		if (!re)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}

		{
			std::unique_lock<std::mutex>lock(mux_);
			if (decode_.Recv(frame_))
			{
				std::cout << "@" << std::flush;
				need_view_ = true;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	{
		std::unique_lock<std::mutex>lock(mux_);
		if (frame_)
		{
			av_frame_free(&frame_);
		}
	}
}

void DecodeTask::Do(AVPacket* pkt)
{
	std::cout << "#" << std::flush;
	if (!pkt||pkt->stream_index != 0)//判断是否为视频流
	{
		return;
	}
	pkt_list_.Push(pkt);
}

bool DecodeTask::Open(AVCodecParameters* para)
{
	if (!para)
	{
		LOGERROR("para is null");
		return false;
	}
	std::unique_lock<std::mutex>lock(mux_);
	auto c = decode_.Create(para->codec_id,false);
	if (!c)
	{
		LOGERROR("ecode_.Create failed");
		return false;
	}
	//复制视频参数
	avcodec_parameters_to_context(c, para);
	decode_.set_c(c);
	if (!decode_.Open())
	{
		LOGERROR("decode_.Open() failed");
		return false;
	}
	LOGINFO("Open decode success");
	return true;
}

AVFrame* DecodeTask::GetFrame()
{
	std::unique_lock<std::mutex>lock(mux_);
	if (!need_view_||!frame_||!frame_->buf[0])return nullptr;
	auto f = av_frame_alloc();
	auto re = av_frame_ref(f, frame_);//引用加一
	if (re != 0)
	{
		av_frame_free(&f);
		PrintErr(re);
		return nullptr;
	}
	need_view_ = false;
	return f;
}
