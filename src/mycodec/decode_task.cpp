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

	long long cur_pts = -1;//当前解码到的pts

	
	while (!is_exit_)
	{
		//同步
		while (!is_exit_)
		{
			if (syn_pts_ >= 0 && cur_pts > syn_pts_)
			{
				MSleep(1);
				continue;
			}
			break;
		}

		auto pkt = pkt_list_.Pop();
		if (!pkt)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}
		//发送到解码线程
		bool re = decode_.Send(pkt);
		if (re) av_packet_free(&pkt);
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
				cur_pts = frame_->pts;
			}
			if (frame_cache_)
			{
				auto f = av_frame_alloc();
				av_frame_ref(f, frame_);
				frames_.push_back(f);
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
	if (!pkt||pkt->stream_index != stream_index_)//判断是否为视频流
	{
		return;
	}
	pkt_list_.Push(pkt);
	if (block_size_ <= 0)return;
	while (!is_exit_)
	{
		if (pkt_list_.Size() > block_size_)
		{
			MSleep(1);
			continue;
		}
		break;
	}
}

bool DecodeTask::Open(AVCodecParameters* para)
{
	if (!para)
	{
		LOGERROR("para is null");
		return false;
	}
	std::unique_lock<std::mutex>lock(mux_);
	is_open_ = false;
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
	is_open_ = true;
	return true;
}

AVFrame* DecodeTask::GetFrame()
{
	std::unique_lock<std::mutex>lock(mux_);
	if (frame_cache_)
	{
		if (frames_.empty())return nullptr;
		auto f = frames_.front();
		frames_.pop_front();
		return f;
	}
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
void DecodeTask::Stop()
{
	BaseThread::Stop();
	pkt_list_.Clear();
	std::unique_lock<std::mutex>lock(mux_);
	decode_.set_c(nullptr);
	is_open_ = false;
	while (!frames_.empty())
	{
		av_frame_free(&frames_.front());
		frames_.pop_front();
	}
}
