#include "decode.h"
#include <thread>
#include <iostream>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}

bool Decode::Send(const AVPacket* pkt)
{
    std::unique_lock<std::mutex>lock(mux_);
    if (!c_) return false;
    auto re = avcodec_send_packet(c_,pkt);
    if (re != 0)return false;
    return true;
}

bool Decode::Recv(AVFrame* frame,bool is_hw_copy)
{
	std::unique_lock<std::mutex>lock(mux_);
	if (!c_) return false;
	auto f = frame;
	if (is_hw_copy&&c_->hw_device_ctx)//硬件加速
	{
		f = av_frame_alloc();
	}
	auto re = avcodec_receive_frame(c_, f);
	if (re == 0)
	{
		if (is_hw_copy&&c_->hw_device_ctx)//GPU解码
		{
			//显存转内存
			re = av_hwframe_transfer_data(frame, f, 0);
			av_frame_free(&f);
			if (re != 0)
			{
				PrintErr(re);
				return false;
			}
		}
		return true;
	}
	if (is_hw_copy&&c_->hw_device_ctx) av_frame_free(&f);
	return false;
}

std::vector<AVFrame*> Decode::End()
{
	std::vector<AVFrame*>res;
	std::unique_lock<std::mutex>lock(mux_);
	if (!c_)return res;
	int ret = avcodec_send_packet(c_, NULL);
	while (ret >= 0)
	{
		auto frame = av_frame_alloc();
		ret = avcodec_send_frame(c_, frame);
		if (ret < 0)
		{
			av_frame_free(&frame);
			break;
		}
		res.push_back(frame);
	}
	return res;
}

bool Decode::InitHW(int type)
{
	std::unique_lock<std::mutex>lock(mux_);
	if (!c_) return false;
	AVBufferRef* ctx = nullptr;//硬件加速上下文
	auto re = av_hwdevice_ctx_create(&ctx, (AVHWDeviceType)type, NULL, NULL,0);
	if (re != 0)
	{
		PrintErr(re);
		return false;
	}
	c_->hw_device_ctx = ctx;
	std::cout << "硬件加速：" << type << std::endl;
	return true;
}
