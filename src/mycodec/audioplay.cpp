#include "audioplay.h"
#include <SDL2/SDL.h>
#include <iostream>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
}

class AAudioPlay:public AudioPlay
{
public:
	bool Open(AAudioSpec& spec) 
	{
		this->spec_ = spec;
		//退出上一次音频处理
		SDL_QuitSubSystem(SDL_INIT_AUDIO);

		SDL_AudioSpec sdl_spec;
		sdl_spec.freq = spec.freq;
		sdl_spec.format = spec.format;
		sdl_spec.channels = spec.channels;
		sdl_spec.samples = spec.samples;
		sdl_spec.silence=0;
		sdl_spec.userdata=this;
		sdl_spec.callback= AudioCallback;
		if (SDL_OpenAudio(&sdl_spec, nullptr) < 0)
		{
			std::cerr << SDL_GetError() << std::endl;
			return false;
		}
		//开始播放
		SDL_PauseAudio(0);
		return true;
	}
	void Close()
	{
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		std::unique_lock<std::mutex>lock(mux_);
		audio_datas_.clear();
	}
	void Callback(unsigned char* stream, int len)
	{
		//清空输出缓冲区，防止残留数据造成噪音
		SDL_memset(stream, 0, len);
		std::unique_lock<std::mutex>lock(mux_);
		if (audio_datas_.empty())return;
		auto buf = audio_datas_.front();
		// 1 buf 大于stream缓冲  offset记录位置
		// 2 buf 小于stream缓冲  拼接
		int mixed_size = 0;//已经处理的字节数
		int need_size = len; //需要处理的字节数
		cur_pts_ = buf.pts;//记录当前播放的pts
		last_ms_ = NowMs();

		while (mixed_size < len)
		{
			if (audio_datas_.empty())break;
			buf = audio_datas_.front();
			int size = buf.data.size() - buf.offset;//当前buffer剩余可用数据量
			//数据充足（size > need_size）：仅取需要的部分
			//数据不足（size <= need_size）：全量取出并准备处理下一buffer
			if (size > need_size)
			{
				size = need_size;
			}
			// 处理每个音频缓冲区
			SDL_MixAudio(stream + mixed_size, buf.data.data() + buf.offset, size, volume_);
			need_size -= size;//剩余需求空间缩减
			mixed_size += size;//已混合数据空间增长
			buf.offset += size;//当前buffer偏移量前进
			if (buf.offset >= buf.data.size())
			{
				audio_datas_.pop_front();
			}
		}
	}
	long long cur_pts() override
	{
		double ms = 0;
		if (last_ms_ > 0) ms - NowMs() - last_ms_;//距离上次写入缓冲的播放时间
		//pts换算成毫秒
		if(time_base_>0)
			ms = ms / (double)1000 / (double)time_base_;
		return cur_pts_ + ms;
	}
private:
	long long cur_pts_ = 0;//当前播放位置
	long long last_ms_ = 0;//上次的时间戳
};

bool AudioPlay::Open(AVCodecParameters* para)
{
	AAudioSpec spec;
	//获取声道布局
	spec.channels = para->ch_layout.nb_channels;
	spec.freq = para->sample_rate;
	switch (para->format)
	{
	case AV_SAMPLE_FMT_S16:
	case AV_SAMPLE_FMT_S16P:
		spec.format = AUDIO_S16;
		break;
	case AV_SAMPLE_FMT_S32:
	case AV_SAMPLE_FMT_S32P:
		spec.format = AUDIO_S32;
		break;
	case AV_SAMPLE_FMT_FLT:
	case AV_SAMPLE_FMT_FLTP:
		spec.format = AUDIO_F32;
		break;
	default:
		break;
	}
	return Open(spec);
}

bool AudioPlay::Open(BasePara& para)
{
	if(para.time_base->num>0)
		time_base_ = (double)para.time_base->den / (double)para.time_base->num;
	return Open(para.para);
}

AudioPlay* AudioPlay::Instance()
{
	static AAudioPlay audioplay;
	return &audioplay;
}

void AudioPlay::Push(AVFrame* frame)
{
	if (!frame || !frame->data[0])return;
	std::vector<unsigned char>buf;
	int sample_size = av_get_bytes_per_sample((AVSampleFormat)frame->format);
	int channels = frame->ch_layout.nb_channels;
	unsigned char* l = frame->data[0];
	unsigned char* r = frame->data[1];
	unsigned char* data = nullptr;
	if (channels == 1)
	{
		Push(frame->data[0], frame->nb_samples*sample_size, frame->pts);
		return;
	}
	//双通道
	switch (frame->format)
	{
	case AV_SAMPLE_FMT_S16P:
	case AV_SAMPLE_FMT_S32P:
	case AV_SAMPLE_FMT_FLTP:
		buf.resize(frame->linesize[0]);
		data = buf.data();
		for (int i = 0; i < frame->nb_samples;i++)
		{
			memcpy(data + i * sample_size * channels, l + i * sample_size,sample_size);
			memcpy(data + i * sample_size * channels+ sample_size, r + i * sample_size,sample_size);
		}
		Push(data, frame->linesize[0],frame->pts);
		return;
		break;
	default:
		break;
	}
	//交叉格式
	Push(frame->data[0], frame->linesize[0], frame->pts);
	return;
}

AudioPlay::AudioPlay()
{
	SDL_Init(SDL_INIT_AUDIO);
}
