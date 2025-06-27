#pragma once
#include "tools.h"
#include "demux.h"
class WEBCAM_API DemuxTask :public BaseThread
{
public:
	void Main();

	/**
	 * 打开解封装.
	 * 
	 * \param url rtsp地址
	 * \param timeout_ms 超时时间
	 * \return 
	 */
	bool Open(std::string url,int timeout_ms = 1000);

	//返回智能指针，复制视频参数
	std::shared_ptr<BasePara> CopyVideoPara()
	{
		return demux_.CopyVideoPara();
	}
	std::shared_ptr<BasePara> CopyAudioPara()
	{
		return demux_.CopyAudioPara();
	}

private:
	Demux demux_;
	std::string url_;
	int timeout_ms_ = 0; //超时时间
};

