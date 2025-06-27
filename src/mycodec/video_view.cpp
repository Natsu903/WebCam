#include "video_view.h"
#include "private_sdl.h"
#include <thread>
#include <iostream>
using namespace std;

extern "C"
{
#include <libavcodec/avcodec.h>
}
#pragma comment(lib,"avutil.lib")


Video_View* Video_View::Create(RenderType type)
{
	switch (type)
	{
	case Video_View::SDL:
		return new PrivateSDL();
		break;
	default:
		break;
	}
	return nullptr;
}

bool Video_View::Init(AVCodecParameters* para)
{
	if (!para)return false;
	auto fmt = (Format)para->format;
	switch (para->format)
	{
	case AV_PIX_FMT_YUV420P:
	case AV_PIX_FMT_YUVJ420P:
		fmt = YUV420P;
		break;
	default:
		break;
	}
	return Init(para->width,para->height,fmt);
}

bool Video_View::DrawFrame(AVFrame* frame)
{
	if (!frame || !frame->data[0])return false;
	count_++;
	//记录当前FPS计算的时间起点。若首次调用（beg_ms_ <= 0），初始化为当前系统时间
	if (beg_ms_ <= 0)
	{
		beg_ms_ = clock();
	}
	//当累计时间超过1000ms（1秒）时,记录过去1秒内的平均帧率（count_值),
	//重置count_为0，更新beg_ms_为当前时间，开始下一秒的帧数统计
	else if ((clock()-beg_ms_)/(CLOCKS_PER_SEC/1000)>=1000)
	{
		render_fps_ = count_;
		count_ = 0;
		beg_ms_ = clock();
	}
	int linesize = 0;
	switch (frame->format)
	{
	case AV_PIX_FMT_YUV420P:
	case AV_PIX_FMT_YUVJ420P:
		return Draw(
			frame->data[0], frame->linesize[0],
			frame->data[1], frame->linesize[1],
			frame->data[2], frame->linesize[2]);
	case AV_PIX_FMT_NV12:
		if (!cache_)
		{
			cache_ = new unsigned char[frame->width * frame->height * 3 / 2];
		}
		linesize = frame->width;
		//如果 Y 分量每行的字节数等于视频帧的宽度，说明 Y 分量数据是连续存储的，可以直接进行整块复制。
		if (frame->linesize[0] == frame->width)
		{
			memcpy(cache_, frame->data[0], frame->linesize[0] * frame->height);//Y
			//将 UV 分量数据从 frame->data[1] 复制到缓存区 cache_ 中，UV 分量数据紧跟在 Y 分量数据之后。
			memcpy(cache_ + frame->linesize[0] * frame->height, frame->data[1], frame->linesize[1] * frame->height / 2);//UV
		}
		//如果 Y 分量每行的字节数不等于视频帧的宽度，说明 Y 分量数据是逐行存储的，需要进行逐行复制。
		else
		{
			//第一个 for 循环：遍历每一行 Y 分量数据，将其从 frame->data[0]复制到缓存区 cache_ 中对应的位置。
			for (int i = 0; i < frame->height; i++)
			{
				memcpy(cache_ + i * frame->width, frame->data[0] + i * frame->linesize[0],frame->width);
			}
			//第二个 for 循环：遍历每一行 UV 分量数据，将其从 frame->data[1]复制到缓存区 cache_ 中对应的位置。
			//需要注意的是，UV 分量数据在缓存区中的起始位置是 cache_ + frame->height * frame->width，
			//因为 Y 分量数据占用了前面 frame->height * frame->width 个字节的空间。
			for (int i =0;i<frame->height/2;i++)//UV
			{
				auto p = cache_ + frame->height * frame->width;//移位Y
				memcpy(p + i * frame->width, frame->data[1] + i * frame->linesize[1], frame->width);
			}
		}
		return Draw(cache_, linesize);
	case AV_PIX_FMT_BGRA:
	case AV_PIX_FMT_ARGB:
	case AV_PIX_FMT_RGBA:
		return Draw(frame->data[0], frame->linesize[0]);
	default:
		break;
	}
	return true;
}

bool Video_View::Open(std::string filepath)
{
	if (ifs_.is_open())
	{
		ifs_.close();
	}
	ifs_.open(filepath, ios::binary);
	return ifs_.is_open();
}

AVFrame* Video_View::Read()
{
	if (width_ <= 0 || height_ <= 0 || !ifs_) return NULL;
	if (frame_)
	{
		if (frame_->width != width_ || frame_->height != height_ || frame_->format != format_)
		{
			av_frame_free(&frame_);
		}
	}
	if (!frame_)
	{
		frame_ = av_frame_alloc();
		frame_->width = width_;
		frame_->height = height_;
		frame_->format = format_;
		frame_->linesize[0] = width_ * 4;
		if (frame_->format == AV_PIX_FMT_YUV420P)
		{
			frame_->linesize[0] = width_;
			frame_->linesize[1] = width_ / 2;
			frame_->linesize[2] = width_ / 2;
		}
		auto re = av_frame_get_buffer(frame_,0);
		if (re != 0)
			if (re != 0)
			{
				char buf[1024] = { 0 };
				av_strerror(re, buf, sizeof(buf) - 1);
				std::cout << buf << std::endl;
				av_frame_free(&frame_);
				return NULL;
			}
	}
	if (!frame_)return NULL;

	//yuv420p读取
	if (frame_->format == AV_PIX_FMT_YUV420P)
	{
		ifs_.read((char*)frame_->data[0],frame_->linesize[0]*height_);
		ifs_.read((char*)frame_->data[1],frame_->linesize[1]*height_/2);
		ifs_.read((char*)frame_->data[2],frame_->linesize[2]*height_/2);
	}
	else //RGBA ARGB BGRA 32
	{
		ifs_.read((char*)frame_->data[0], frame_->linesize[0] * height_);
	}
	//获取最后一次读取操作的实际字节数。若为 0，表示未读取到有效数据（如流结束），返回 NULL
	if (ifs_.gcount() == 0) return NULL;
	return frame_;
}

Video_View::~Video_View()
{
	if (cache_)delete cache_;
	cache_ = nullptr;
}
