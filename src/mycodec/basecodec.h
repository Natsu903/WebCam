#pragma once
#include <mutex>
#include <vector>
#include "tools.h"

/**
 * 编码和解码的基类
 */
class WEBCAM_API Codec
{
public:

	/**
	 * 初始化编码器上下文.
	 *
	 * \param codec_id 传入编码器id如AV_CODEC_ID_H264
	 * \return 返回配置好的AVCodecContext指针
	 */
	static AVCodecContext* Create(int codec_id, bool is_encode);

	/**
	 * 将编码器上下文存储到类中.
	 *
	 * \param c
	 */
	void set_c(AVCodecContext* c);

	/**
	 * 设置ffmpeg编码器选项参数.
	 *
	 * \param key 参数类型
	 * \param val 参数
	 * \return 成功返回true失败返回false
	 */
	bool SetOpt(const char* key, const char* val);
	bool SetOpt(const char* key, int val);

	/**
	 * 初始化编解码器上下文并与具体的编解码器关联.
	 *
	 * \return 成功返回true
	 */
	bool Open();

	AVFrame* CreateFrame();

protected:
	AVCodecContext* c_ = nullptr;
	std::mutex mux_;
};

