#include "camera_record.h"
#include "demux_task.h"
#include "mux_task.h"
#include <chrono>
#include <iomanip>
#include <sstream>

//生成存储的视频文件名
static std::string GetFileName(std::string path)
{
	std::stringstream ss;
	auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

#if defined(_MSC_VER)
	// Windows 下使用 localtime_s
	struct tm tm_buf;
	localtime_s(&tm_buf, &time);
	const struct tm* tm_ptr = &tm_buf;
#else
	// Linux/Unix 使用 localtime_r
	struct tm tm_buf;
	localtime_r(&time, &tm_buf);
	const struct tm* tm_ptr = &tm_buf;
#endif
	auto time_str = std::put_time(tm_ptr, "%Y_%m_%d_%H_%M_%S");
	ss << path << "/" << "cam_" << time_str << ".mp4";
	return ss.str();
}

void CameraRecord::Main()
{
	DemuxTask demux;
	MuxTask mux;
	if (rtsp_url_.empty())
	{
		LOGERROR("open rtsp url failed");
		return;
	}
	while (!is_exit_)
	{
		if (demux.Open(rtsp_url_))
		{
			break;
		}
		MSleep(3000);
		continue;
	}
	//获取音视频参数
	auto vpara = demux.CopyVideoPara();
	if (!vpara)
	{
		LOGERROR("demux.CopyVideoPara() failed")
		demux.Stop();
		return;
	}
	//启动解封装
	demux.Start();
	auto apara = demux.CopyAudioPara();

	AVCodecParameters* para = nullptr;//音频参数
	AVRational* timebase = nullptr;//音频时间基数
	if (apara)
	{
		para = apara->para;
		timebase = apara->time_base;
	}

	if (!mux.Open(GetFileName(save_path_).c_str(), vpara->para, vpara->time_base, para, timebase))
	{
		LOGERROR("mux.Open failed")
		demux.Stop();
		mux.Stop();
		return;
	}
	demux.set_next(&mux);
	mux.Start();

	auto cur = NowMs();
	while (!is_exit_)
	{
		//定时创建新文件
		if (NowMs() - cur > file_sec_ * 1000)
		{
			cur = NowMs();
			mux.Stop();//停止存储写入索引
			if (!mux.Open(GetFileName(save_path_).c_str(), vpara->para, vpara->time_base, para, timebase))
			{
				LOGERROR("mux.Open failed");
				demux.Stop();
				mux.Stop();
				return;
			}
			mux.Start();
		}
		MSleep(10);
	}
	mux.Stop();
	demux.Stop();
}
