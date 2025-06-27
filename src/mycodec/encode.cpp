#include "encode.h"
#include <thread>
#include <iostream>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}


AVPacket* Encode::DoEncode(const AVFrame* frame)
{
    if (!frame)return nullptr;
    std::unique_lock<std::mutex>lock(mux_);
    if (!c_) return nullptr;
    auto re = avcodec_send_frame(c_,frame);
    if (re != 0)return nullptr;
    auto pkt = av_packet_alloc();
    re = avcodec_receive_packet(c_, pkt);
    //接收成功则返回pkt
    if (re == 0)
    {
        return pkt;
    }
    //接收失败，执行错误处理
    av_packet_free(&pkt);
    //如果是处理结束返回空
	if (re == AVERROR(EAGAIN) || re == AVERROR_EOF)
	{
		return nullptr;
	}
    //打印错误代码
	if (re < 0)
	{
		PrintErr(re);
	}
	return nullptr;
}

std::vector<AVPacket*> Encode::End()
{
    std::vector<AVPacket*>res;
    std::unique_lock<std::mutex>lock(mux_);
    if (!c_)return res;
    auto re = avcodec_send_frame(c_, NULL);
    while (re >= 0)
    {
        auto pkt = av_packet_alloc();
        re = avcodec_receive_packet(c_, pkt);
        if (re != 0)
        {
            av_packet_free(&pkt);
            break;
        }
        res.push_back(pkt);
    }
    return res;
}
