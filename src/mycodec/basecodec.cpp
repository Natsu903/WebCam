#include "basecodec.h"
#include <thread>
#include <iostream>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}

#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avutil.lib")

AVCodecContext* Codec::Create(int codec_id,bool is_encode)
{
	//查找编码器
	const AVCodec* codec = nullptr;
	if (is_encode)
	{
		codec = avcodec_find_encoder((AVCodecID)codec_id);
	}
	else
	{
		codec = avcodec_find_decoder((AVCodecID)codec_id);
	}
	if (!codec)
	{
		std::cerr << "avcodec_find_encoder error" << std::endl;
	}
	//创建编码器上下文
	auto c = avcodec_alloc_context3(codec);
	if (!c)
	{
		std::cerr << "avcodec_alloc_context3 failed" << std::endl;
	}
	c->time_base = { 1,25 };
	c->pix_fmt = AV_PIX_FMT_YUV420P;
	c->thread_count = std::thread::hardware_concurrency();
	return c;
}


void Codec::set_c(AVCodecContext* c)
{
	std::unique_lock<std::mutex>lock(mux_);
	if (c_)
	{
		avcodec_free_context(&c_);
	}
	this->c_ = c;
}

bool Codec::SetOpt(const char* key, const char* val)
{
	std::unique_lock<std::mutex>lock(mux_);
	if (!c_)return false;
	auto re = av_opt_set(c_->priv_data, key, val, 0);
	if (re != 0)
	{
		std::cerr << "SetOpt by char val failed" << std::endl;
		PrintErr(re);
		return false;
	}
	return true;
}

bool Codec::SetOpt(const char* key, int val)
{
	std::unique_lock<std::mutex>lock(mux_);
	if (!c_)return false;
	auto re = av_opt_set_int(c_->priv_data, key, val, 0);
	if (re != 0)
	{
		std::cerr << "SetOpt by int val failed" << std::endl;
		PrintErr(re);
		return false;
	}
	return true;
}

bool Codec::Open()
{
	std::unique_lock<std::mutex>lock(mux_);
	if (!c_) return false;
	auto re = avcodec_open2(c_, NULL, NULL);
	if (re != 0)
	{
		PrintErr(re);
		return false;
	}
	return true;
}

AVFrame* Codec::CreateFrame()
{
	std::unique_lock<std::mutex>lock(mux_);
	if (!c_)return nullptr;
	auto frame = av_frame_alloc();
	frame->width = c_->width;
	frame->height = c_->height;
	frame->format = c_->pix_fmt;
	auto re = av_frame_get_buffer(frame, 0);
	if (re != 0)
	{
		PrintErr(re);
		return nullptr;
	}
	return frame;
}
