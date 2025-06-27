#pragma once
#include "basecodec.h"

class WEBCAM_API Encode:public Codec
{
public:
	
	/**
	 * 处理帧画面.
	 * 
	 * \param frame创建好的frame
	 * \return 处理成功则返回pkt，处理到结尾返回nullptr，处理失败打印错误信息
	 */
	AVPacket* DoEncode(const AVFrame* frame);

	std::vector<AVPacket*>End();

};

