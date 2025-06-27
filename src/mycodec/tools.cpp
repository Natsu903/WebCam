#include "tools.h"
#include "sstream"
#include <iostream>
#include <chrono>
extern "C"
{
#include <libavcodec/avcodec.h>
}

void BaseThread::Start()
{
	std::unique_lock<std::mutex>lock(m_);
	static int i = 0;
	i++;
	index_ = i;
	is_exit_ = false;
	//启动线程
	th_ = std::thread(&BaseThread::Main, this);
	std::stringstream ss;
	ss << "Demuxthread::Start()" << index_;
	LOGINFO(ss.str());
}

void BaseThread::Stop()
{
	std::stringstream ss;
	ss << "Demuxthread::Stop() begin" << index_;
	LOGINFO(ss.str());
	is_exit_ = true;
	if (th_.joinable())
	{
		th_.join();
	}
	ss.str("");
	ss << "Demuxthread::Stop() end" << index_;
	LOGINFO(ss.str());

}

BasePara* BasePara::Create()
{
	return new BasePara();
}

BasePara::~BasePara()
{
	if (para)avcodec_parameters_free(&para);
	if (time_base)
	{
		delete time_base;
		time_base = nullptr;
	}
}

BasePara::BasePara()
{
	para = avcodec_parameters_alloc();
	time_base = new AVRational();
}

AVPacket* SafetyAVPacketList::Pop()
{
	std::unique_lock<std::mutex>lock(mux_);
	if (pkts_.empty())return nullptr;
	auto pkt = pkts_.front();
	pkts_.pop_front();
	return pkt;
}

void SafetyAVPacketList::Push(AVPacket* pkt)
{
	std::unique_lock<std::mutex>lock(mux_);
	//生成新的AVPacket对象，引用技术+1
	auto p = av_packet_alloc();
	av_packet_ref(p, pkt);//引用计数，减少数据复制，线程安全
	pkts_.push_back(p);
	//超出最大空间，清理数据到关键帧位置
	if (pkts_.size() > max_packets_)
	{
		//处理第一帧
		if (pkts_.front()->flags & AV_PKT_FLAG_KEY)//关键帧
		{
			av_packet_free(&pkts_.front());//清理
			pkts_.pop_front();//出栈
			return;
		}
		//清理所有非关键帧数据
		while (!pkts_.empty())
		{
			if (pkts_.front()->flags & AV_PKT_FLAG_KEY)//关键帧
			{
				return;
			}
			av_packet_free(&pkts_.front());//清理
			pkts_.pop_front();//出栈
		}
	}
}

void MSleep(unsigned int ms)
{
	//高精度计时器初始化
	//使用std::chrono::high_resolution_clock获取当前时间点beg，作为计时起点。
	//该时钟提供系统支持的最高精度（通常是纳秒或微秒级）
	auto beg = std::chrono::high_resolution_clock::now();
	while (true) {
		//在无限循环中，每次迭代通过now记录当前时间点，用于计算已流逝的时间
		auto now = std::chrono::high_resolution_clock::now();
		//​计算时间差
		//now - beg：得到时间间隔（duration类型），单位为时钟周期。
		//duration_cast<std::chrono::milliseconds>：将时间间隔转换为毫秒级整数值
		//.count()：提取毫秒数的整数值
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - beg).count();
		//若累计时间elapsed达到或超过目标延迟ms，则退出循环
		if (elapsed >= ms) break;
		//每次循环休眠1毫秒
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

long long NowMs()
{
	return clock() / (CLOCKS_PER_SEC / 1000);
}

void FreeFrame(AVFrame** frame)
{
	if (!frame||!(*frame))return;
	av_frame_free(frame);
}

void PrintErr(int err)
{
	char buf[1024] = { 0 };
	av_strerror(err, buf, sizeof(buf) - 1);
	std::cerr << buf << std::endl;
}
