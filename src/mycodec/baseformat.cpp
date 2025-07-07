#include "baseformat.h"
#include <iostream>
#include <thread>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"avcodec.lib")

static int TimeoutCallback(void* para)
{
    auto bf = (BaseFormat*)para;
    if (bf->IsTimeout())return -1;//超时退出-1
    return 0;
}

void BaseFormat::set_c(AVFormatContext* c)
{
	std::unique_lock<std::mutex>lock(mux_);
    if (c_)//清理原值
    {
        if (c_->oformat)//输出上下文
        {
            if (c_->pb)
                avio_closep(&c_->pb);
            avformat_free_context(c_);
        }
        else if (c_->iformat)//输入上下文
        {
            avformat_close_input(&c_);
        }
        else 
        {
            avformat_free_context(c_);
        }
    }
    c_ = c;
    if (!c_)
    {
        is_connected_ = false;
        return;
    }
    is_connected_ = true;

    //计时用于超时判断
    last_time_ = NowMs();

    if (timeout_ms_ > 0)
    {
		AVIOInterruptCB cb = { TimeoutCallback ,this };
		c_->interrupt_callback = cb;
    }

    //区分是否有音频视频流
    audio_index_ = -1;
    video_index_ = -1;
    //区分音视频索引
    for (int i = 0; i < c->nb_streams; i++)
    {
        if (c->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            audio_index_ = i;
            audio_time_base_.den = c->streams[i]->time_base.den;
            audio_time_base_.num = c->streams[i]->time_base.num;
        }
        else if (c->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_index_ = i;
            video_time_base_.den = c->streams[i]->time_base.den;
            video_time_base_.num = c->streams[i]->time_base.num;
            video_codec_id_ = c->streams[i]->codecpar->codec_id;
        }
    }
}

bool BaseFormat::CopyPara(int stream_index, AVCodecParameters* dst)
{
    std::unique_lock<std::mutex>lock(mux_);
    if (!c_)return false;
    if (stream_index<0 || stream_index>c_->nb_streams) return false;
    auto re = avcodec_parameters_copy(dst, c_->streams[stream_index]->codecpar);
    if (re < 0)return false;
    return true;
}

bool BaseFormat::CopyPara(int stream_index, AVCodecContext* dst)
{
	std::unique_lock<std::mutex>lock(mux_);
	if (!c_)return false;
    if (stream_index<0 || stream_index>c_->nb_streams) return false;
    auto re = avcodec_parameters_to_context(dst, c_->streams[stream_index]->codecpar);
    return false;
}

std::shared_ptr<BasePara> BaseFormat::CopyVideoPara()
{
    int index = video_index();
    std::shared_ptr<BasePara> re;
    std::unique_lock<std::mutex>lock(mux_);
    if (index < 0||!c_)return re;
    re.reset(BasePara::Create());
    *re->time_base = c_->streams[index]->time_base;
    avcodec_parameters_copy(re->para, c_->streams[index]->codecpar);
    return re;
}

std::shared_ptr<BasePara> BaseFormat::CopyAudioPara()
{
	int index = audio_index();
	std::shared_ptr<BasePara> re;
	std::unique_lock<std::mutex>lock(mux_);
	if (index < 0 || !c_)return re;
	re.reset(BasePara::Create());
	*re->time_base = c_->streams[index]->time_base;
	avcodec_parameters_copy(re->para, c_->streams[index]->codecpar);
	return re;
}

bool BaseFormat::RescaleTime(AVPacket* pkt, long long offset_pts, XRational time_base)
{
    AVRational in_time_base;
    in_time_base.num = time_base.num;
    in_time_base.den = time_base.den;
    return RescaleTime(pkt, offset_pts, &in_time_base);
}

bool BaseFormat::RescaleTime(AVPacket* pkt, long long offset_pts, AVRational* time_base)
{
    if (!pkt || !time_base)return false;
	std::unique_lock<std::mutex>lock(mux_);
	if (!c_)return false;
	auto out_stream = c_->streams[pkt->stream_index];
	pkt->pts = av_rescale_q_rnd(pkt->pts - offset_pts, *time_base,
		out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX)
	);
	pkt->dts = av_rescale_q_rnd(pkt->dts - offset_pts, *time_base,
		out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX)
	);
	pkt->duration = av_rescale_q(pkt->duration, *time_base, out_stream->time_base);
	pkt->pos = -1;
	return true;
}

long long BaseFormat::RescaleToMs(long long pts, int index)
{
    std::unique_lock<std::mutex> lock(mux_);
    if (!c_||index<0||index>c_->nb_streams)return 0;
    auto in_timebase = c_->streams[index]->time_base;
    AVRational out_timebase = { 1,1000 };
	return av_rescale_q(pts, in_timebase, out_timebase);
}

void BaseFormat::set_timeout_ms(int ms)
{
    std::unique_lock<std::mutex>lock(mux_);
    this->timeout_ms_ = ms;
    if (c_)
    {
        AVIOInterruptCB cb = { TimeoutCallback,this };
        c_->interrupt_callback = cb;
    }
}
