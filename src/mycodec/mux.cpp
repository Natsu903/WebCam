#include "mux.h"
#include <iostream>
#include <thread>

extern "C"
{
#include <libavformat/avformat.h>
}

void PrintErr(int err);

#define BERR(err) if(err!= 0){PrintErr(err);return 0;}

AVFormatContext* Mux::Open(const char* url,AVCodecParameters* video_para,AVCodecParameters* audio_para)
{
	//创建上下文
	AVFormatContext* c = nullptr;
	auto re = avformat_alloc_output_context2(&c, NULL, NULL, url);
	BERR(re);
	//添加音视频流
	if (video_para)
	{
		auto vs = avformat_new_stream(c, NULL);
		avcodec_parameters_copy(vs->codecpar, video_para);
	}
	if (audio_para)
	{
		auto as = avformat_new_stream(c, NULL);
		avcodec_parameters_copy(as->codecpar, audio_para);
	}

	//打开IO
	re = avio_open(&c->pb, url, AVIO_FLAG_WRITE);
	BERR(re);
	av_dump_format(c, 0, url, 1);
	return c;
}

bool Mux::WriteHead()
{
	std::unique_lock<std::mutex>lock(mux_);
	if (!c_)return false;
	//会改变timebase
	auto re = avformat_write_header(c_, nullptr);
	BERR(re);

	//打印输出上下文
	av_dump_format(c_, 0, c_->url, 1);
	this->begin_video_pts_ = -1;
	this->begin_audio_pts_ = -1;



	return true;
}

bool Mux::Write(AVPacket* pkt)
{
	if (!pkt)return false;
	std::unique_lock<std::mutex>lock(mux_);
	if (!c_)return false;
	//没读取到pts
	if (pkt->pts == AV_NOPTS_VALUE)
	{
		pkt->pts = 0;
		pkt->dts = 0;
	}
	if (pkt->stream_index == video_index_)
	{
		if (begin_video_pts_ < 0)
			begin_video_pts_ = pkt->pts;
		lock.unlock();
		RescaleTime(pkt, begin_video_pts_, src_video_time_base_);
		lock.lock();
	}
	if (pkt->stream_index == audio_index_)
	{
		if (begin_audio_pts_ < 0)
			begin_audio_pts_ = pkt->pts;
		lock.unlock();
		RescaleTime(pkt, begin_audio_pts_, src_audio_time_base_);
		lock.lock();
	}
	std::cout << pkt->pts << " " << std::flush;
	//写入一帧数据，内部缓冲排序dts，通过pkt=null 可以写入缓冲
	auto re = av_interleaved_write_frame(c_, pkt);
	BERR(re);
	return true;
}

bool Mux::WriteEnd()
{
	std::unique_lock<std::mutex>lock(mux_);
	if (!c_)return false;
	int re = 0;
	re = av_write_trailer(c_);
	BERR(re);
	return true;
}

void Mux::set_src_video_time_base(AVRational* tb)
{
	if (!tb)return;
	std::unique_lock<std::mutex>lock(mux_);
	if (!src_video_time_base_)
	{
		src_video_time_base_ = new AVRational();
	}
	*src_video_time_base_ = *tb;
}

void Mux::set_src_audio_time_base(AVRational* tb)
{
	if (!tb)return;
	std::unique_lock<std::mutex>lock(mux_);
	if (!src_audio_time_base_)
	{
		src_audio_time_base_ = new AVRational();
	}
	*src_audio_time_base_ = *tb;
}

Mux::~Mux()
{
	std::unique_lock<std::mutex>lock(mux_);
	if (src_video_time_base_)
	{
		delete src_video_time_base_;
		src_video_time_base_ = nullptr;
	}
	if (src_audio_time_base_)
	{
		delete src_audio_time_base_;
		src_audio_time_base_ = nullptr;
	}
}
