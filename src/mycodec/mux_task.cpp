#include "mux_task.h"
#include <chrono>
extern "C"
{
#include <libavformat/avformat.h>
}


void MuxTask::Main()
{
    xmux_.WriteHead();
    //找到关键帧
    while (!is_exit_)
    {
		std::unique_lock<std::mutex>lock(mux_);
		auto pkt = pkts_.Pop();
		if (!pkt)
		{
			MSleep(1);
			continue;
		}
        //关键帧
        if (pkt->stream_index == xmux_.video_index() && pkt->flags & AV_PKT_FLAG_KEY)
        {
            xmux_.Write(pkt);
            av_packet_free(&pkt);
            break;
        }
        //丢掉非视频关键帧
        av_packet_free(&pkt);
    }

    while (!is_exit_)
    {
        std::unique_lock<std::mutex>lock(mux_);
        auto pkt = pkts_.Pop();
        if (!pkt)
        {
            MSleep(1);
            continue;
        }
        xmux_.Write(pkt);
        std::cout << "W" << std::flush;
        av_packet_free(&pkt);
    }
    xmux_.WriteEnd();
    xmux_.set_c(nullptr);
}

bool MuxTask::Open(const char* url, AVCodecParameters* video_para, AVRational* video_time_base, AVCodecParameters* audio_para, AVRational* audio_time_base)
{
    auto c = xmux_.Open(url,video_para,audio_para);
    if (!c)return false;
    xmux_.set_c(c);
    xmux_.set_src_video_time_base(video_time_base);
    xmux_.set_src_audio_time_base(audio_time_base);
    return true;
}

void MuxTask::Do(AVPacket* pkt)
{
    pkts_.Push(pkt);
    Next(pkt);
}
