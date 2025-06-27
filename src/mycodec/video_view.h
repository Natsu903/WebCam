#pragma once
#include <mutex>
#include <fstream>
#include "tools.h"

class WEBCAM_API Video_View
{
public:
	enum Format
	{
		YUV420P = 0,
		NV12=23,
		ARGB = 25,
		RGBA = 26,
		BGRA = 28

	};
	enum RenderType
	{
		SDL = 0
	};

	static Video_View* Create(RenderType type=SDL);

	/**
	 * 初始化渲染窗口 线程安全.
	 * 
	 * \param w 窗口宽度
	 * \param h 窗口高度
	 * \param fmt 绘制的像素格式，默认为RGBA
	 * \param win_id 窗口句柄，没有就重新创建一个
	 * \return 是否创建成功
	 */
	virtual bool Init(int w, int h, Format fmt = RGBA, void* win_id = nullptr)=0;

	virtual bool Init(AVCodecParameters* para);

	/**
	 * 渲染图像 线程安全.
	 * 
	 * \param data 渲染的二进制图像
	 * \param linesize 一行数组的字节数，若linesize<=0则自动计算大小
	 * \return 渲染是否成功
	 */
	virtual bool Draw(const unsigned char* data, int linesize = 0)=0;
	virtual bool Draw(
		const unsigned char* y,int y_pitch,
		const unsigned char* u, int u_pitch, 
		const unsigned char* v, int v_pitch)=0;

	/**
	 * 绘制接口封装，针对不同绘制需求使用相同的DrawFrame，内部调用不同的Draw函数.
	 * 
	 * \param frame
	 * \return 
	 */
	virtual bool DrawFrame(AVFrame* frame);

	/**
	 * .
	 * 
	 * \return render_fps_
	 */
	int render_fps()
	{
		return render_fps_;
	}
	
	/**
	 * 清理申请的资源，关闭窗口.
	 * 
	 */
	virtual void Close() = 0;

	/**
	 * 处理窗口退出事件.
	 * 
	 * \return 
	 */
	virtual bool IsExit() = 0;

	//窗口缩放
	void Scale(int w, int h)
	{
		scale_w_ = w;
		scale_h_ = h;
	}


	bool Open(std::string filepath);
	//从视频流中读取并解析一帧图像数据
	AVFrame* Read();
	
	void set_win_id(void* win) { win_id_ = win; }

	~Video_View();

protected:
	void* win_id_ = nullptr;
	int render_fps_ = 0;
	int width_=0;		//窗口宽
	int height_=0;		//窗口高
	Format format_=RGBA;//像素格式
	std::mutex mtx_;
	int scale_w_ = 0;
	int scale_h_ = 0;
	long long beg_ms_ = 0;
	int count_ = 0;

private:
	std::ifstream ifs_;
	AVFrame* frame_ = nullptr;
	unsigned char* cache_ = nullptr;
};

