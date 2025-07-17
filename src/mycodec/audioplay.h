#pragma once
#include<vector>
#include<list>
#include<mutex>
#include "tools.h"
/**
 * ffmpeg中定义的格式类型.
 */
#define AUDIO_U8        0x0008  /**< Unsigned 8-bit samples */
#define AUDIO_S8        0x8008  /**< Signed 8-bit samples */
#define AUDIO_U16LSB    0x0010  /**< Unsigned 16-bit samples */
#define AUDIO_S16LSB    0x8010  /**< Signed 16-bit samples */
#define AUDIO_U16MSB    0x1010  /**< As above, but big-endian byte order */
#define AUDIO_S16MSB    0x9010  /**< As above, but big-endian byte order */
#define AUDIO_U16       AUDIO_U16LSB
#define AUDIO_S16       AUDIO_S16LSB
/* @} */

/**
 *  \name int32 support
 */
 /* @{ */
#define AUDIO_S32LSB    0x8020  /**< 32-bit integer samples */
#define AUDIO_S32MSB    0x9020  /**< As above, but big-endian byte order */
#define AUDIO_S32       AUDIO_S32LSB
/* @} */

/**
 *  \name float32 support
 */
 /* @{ */
#define AUDIO_F32LSB    0x8120  /**< 32-bit floating point samples */
#define AUDIO_F32MSB    0x9120  /**< As above, but big-endian byte order */
#define AUDIO_F32       AUDIO_F32LSB
/* @} */

/**
 *  \name Native audio byte ordering
 */
 /* @{ */
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define AUDIO_U16SYS    AUDIO_U16LSB
#define AUDIO_S16SYS    AUDIO_S16LSB
#define AUDIO_S32SYS    AUDIO_S32LSB
#define AUDIO_F32SYS    AUDIO_F32LSB
#else
#define AUDIO_U16SYS    AUDIO_U16MSB
#define AUDIO_S16SYS    AUDIO_S16MSB
#define AUDIO_S32SYS    AUDIO_S32MSB
#define AUDIO_F32SYS    AUDIO_F32MSB
#endif

struct  AAudioSpec
{
	int freq = 44100;//采样率
	unsigned short format = AUDIO_S16SYS;
	unsigned char channels = 2;
	unsigned char samples = 1024;
};

struct DData
{
	std::vector<unsigned char>data;
	int offset = 0;//偏移位置
	long long pts = 0;
};

/**
 * 音频播放.
 */
class WEBCAM_API AudioPlay
{
public:
	//单例模式
	static AudioPlay* Instance();
	virtual void Push(AVFrame* frame);
	//打开音频 开始播放，调用回调函数
	virtual bool Open(AVCodecParameters* para);
	virtual bool Open(BasePara& para);
	virtual bool Open(AAudioSpec& spec) = 0;
	virtual void Close() = 0;
	//获取当前的播放位置
	virtual long long cur_pts() = 0;
	void Push(const unsigned char* data, int size,long long pts)
	{
		std::unique_lock<std::mutex> lock(mux_);
		audio_datas_.push_back(DData());
		audio_datas_.back().pts = pts;
		audio_datas_.back().data.assign(data, data + size);
	}

	//播放速度
	virtual void SetSpeed(float s)
	{
		auto spec = spec_;
		auto old_freq = spec.freq;
		spec.freq *= s;
		Open(spec);
		spec_.freq = old_freq;
	}

	//音量
	void set_volume(int v)
	{
		volume_ = v;
	}
	//时间基数用于生成播放进度
	void set_time_base(double b) { time_base_ = b; }

protected:
	double time_base_ = 0;
	AudioPlay();
	//回调函数用于处理异步
	virtual void Callback(unsigned char* stream, int len) = 0;
	static void AudioCallback(void* userdata, unsigned char* stream, int len)
	{
		auto ap = (AudioPlay*)userdata;
		ap->Callback(stream, len);
	}
	std::list<DData> audio_datas_;
	std::mutex mux_;
	unsigned char volume_ = 128;
	AAudioSpec spec_;
};

