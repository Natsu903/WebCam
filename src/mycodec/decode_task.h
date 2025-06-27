#pragma once
#include "tools.h"
#include "decode.h"
class WEBCAM_API DecodeTask :public BaseThread
{
public:
	//线程主函数
	void Main() override;
	//责任链处理函数
	void Do(AVPacket* pkt) override;
	//打开解码器
	bool Open(AVCodecParameters* para);


	/**	
	* 返回当前需要渲染的Frame, 如果没有返回null
	* need_view_控制渲染
	* 线程安全.
	* 返回结果需要用FreeFrame释放
	*/
	AVFrame* GetFrame();

private:
	std::mutex mux_;
	Decode decode_;
	SafetyAVPacketList pkt_list_;
	AVFrame* frame_ = nullptr;//解码后存储
	bool need_view_ = false;//是否需要渲染，每帧只渲染一次，通过getframe获取
};

