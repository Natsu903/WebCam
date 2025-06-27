#pragma once
#include "basecodec.h"

struct AVPacket;
struct AVFrame;
class WEBCAM_API Decode :public Codec
{
public:
	/**
	 * 发送解码数据.
	 * 
	 * \param pkt 要发送的解码数据
	 * \return 是否发送成功
	 */
	bool Send(const AVPacket* pkt);

	/**
	 * 接收解码数据.
	 * 
	 * \param frame 接收后要存储到的frame
	 * \param is_hw_copy 是否拷贝显存内容到内存
	 * \return 是否接收成功
	 */
	bool Recv(AVFrame* frame, bool is_hw_copy=true);

	/**
	 * 获取缓存.
	 * 
	 * \return 存储为vector的缓存内容
	 */
	std::vector<AVFrame*>End();

	/**
	 * 初始化硬件加速.
	 * 
	 * \param type 硬件加速类型默认为4即 AV_HWDEVICE_TYPE_DXVA2
	 * \return 是否初始化成功
	 */
	bool InitHW(int type=4);
};

