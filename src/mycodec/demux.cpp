#include "demux.h"
#include <iostream>
#include <thread>

extern "C"
{
#include <libavformat/avformat.h>
}

void PrintErr(int err);

#define BERR(err) if(err!= 0){PrintErr(err);return 0;}

AVFormatContext* Demux::Open(const char* url)
{
	//av_log_set_level(AV_LOG_DEBUG);//ffmpeg详细日志
    AVFormatContext* c = nullptr;
    AVDictionary* opts = nullptr;
    //av_dict_set(&opts, "rtsp_transport", "tcp", 0);//传输流媒体协议改为tcp
	/**
	1. timeout
	•	作用对象：底层协议（如 TCP、HTTP、RTMP 等）的连接建立阶段。
	•	单位：微秒（μs）。
	•	典型场景：
	•	TCP 连接超时（如 RTSP 服务未响应）。
	•	HTTP 重定向超时。
	•	RTMP 握手超时。
	2. stimeout
	•	作用对象：上层协议（如 RTSP、RTP、HLS 等）的数据传输阶段。
	•	单位：微秒（μs）。
	•	典型场景：
	•	RTSP 控制命令（如 DESCRIBE、SETUP）无响应。
	•	RTP 数据包接收超时。
	•	HLS 分片加载超时。

		场景			timeout 生效条件				stimeout 生效条件
		TCP 连接失败		✔️（如目标 IP 无法访问）		❌
		RTSP 服务未响应	✔️（RTSP 握手超时）			❌
		RTP 数据包丢失	❌							✔️（等待 RTP 包超时）
		RTMP 握手失败		✔️（协议层连接超时）			❌
		HLS 分片加载缓慢	❌							✔️（分片加载超时）
	 */
    av_dict_set(&opts, "stimeout", "1000000", 0);//RTSP/RTP 数据超时 1 秒
	av_dict_set(&opts, "timeout", "1000000", 0);//tcp连接超时一秒
	auto re = avformat_open_input(&c, url, nullptr, &opts);
	if (opts)
	{
		av_dict_free(&opts);
	}
	BERR(re);
	//获取媒体信息
	re = avformat_find_stream_info(c, nullptr);
	BERR(re);
	//打印封装信息
	av_dump_format(c, 0, url, 0);
	return c;
}

bool Demux::Read(AVPacket* pkt)
{
	std::unique_lock<std::mutex>lock(mux_);
	if (!c_)return false;
    auto re = av_read_frame(c_, pkt);
	BERR(re);
    //计时用于超时判断
    last_time_ = NowMs();
    return true;
}

bool Demux::Seek(long long pts, int stream_index)
{
	std::unique_lock<std::mutex>lock(mux_);
	if (!c_)return false;
    auto re = av_seek_frame(c_, stream_index, pts, AVSEEK_FLAG_FRAME | AVSEEK_FLAG_BACKWARD);
    BERR(re);
    return true;
}

