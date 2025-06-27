#pragma once

//兼容Linux
#ifdef _WIN32
#ifdef WEBCAM_EXPORTS
#define WEBCAM_API __declspec(dllexport)
#else
#define WEBCAM_API __declspec(dllimport)
#endif
#else
#define WEBCAM_API
#endif // _WIN32

#include <thread>
#include <mutex>
#include <iostream>
#include <list>

//日志
enum LogLevel
{
	LOG_TYPE_DEBUG,
	LOG_TYPE_INFO,
	LOG_TYPE_ERROR,
	LOG_TYPE_FATAL
};

#define LOG_MIN_LEVEL LOG_TYPE_DEBUG
#define PRINTLOG(s,level)\
if(level>=LOG_MIN_LEVEL)\
std::cout<<level<<":"<<__FILE__<<":"<<__LINE__<<":\n"\
<<s<<std::endl;

#define LOGDEBUG(s) PRINTLOG(s,LOG_TYPE_DEBUG)
#define LOGINFO(s) PRINTLOG(s,LOG_TYPE_INFO)
#define LOGERROR(s) PRINTLOG(s,LOG_TYPE_ERROR)
#define LOGDEBUG(s) PRINTLOG(s,LOG_TYPE_DEBUG)

struct AVPacket;
struct AVCodecParameters;
struct AVRational;
struct AVFrame;
struct AVCodecContext;

//高精度定时休眠
WEBCAM_API void MSleep(unsigned int ms);

WEBCAM_API long long NowMs();

WEBCAM_API void FreeFrame(AVFrame** frame);

WEBCAM_API void PrintErr(int err);

class WEBCAM_API BaseThread
{
public:
	//启动线程
	virtual void Start();

	//结束线程
	virtual void Stop();

	//执行任务，需要重载
	virtual void Do(AVPacket* pkt){}

	//传递到下个责任链函数
	virtual void Next(AVPacket* pkt)
	{
		std::unique_lock<std::mutex> lock(m_);
		if (next_)
		{
			next_->Do(pkt);
		}
	}

	//设置责任链下一个节点
	void set_next(BaseThread* bt)
	{
		std::unique_lock<std::mutex> lock(m_);
		next_ = bt;
	}

protected:
	//线程入口函数
	virtual void Main()=0;

	//标志线程退出
	bool is_exit_ = false;

	//线程索引号
	int index_ = 0;

private:
	std::thread th_;
	std::mutex m_;
	BaseThread* next_ = nullptr;//责任链下一个节点
};

class Tools
{
};

//音视频参数
class WEBCAM_API BasePara
{
public:
	AVCodecParameters* para = nullptr;//音视频参数
	AVRational* time_base = nullptr;//时间基数

	//创建对象
	static BasePara* Create();
	~BasePara();
private:
	//禁止创建栈中对象
	BasePara();
};

/**
 * 线程安全avpacket list.
 * 责任链收到数据先交由到列表中，线程再从列表中取得处理
 */
class WEBCAM_API SafetyAVPacketList
{
public:
	AVPacket* Pop();
	void Push(AVPacket* pkt);

private:
	std::list<AVPacket*> pkts_;
	int max_packets_ = 100;//最大列表数量，超出清理
	std::mutex mux_;
};
