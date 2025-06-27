#pragma once
#include "video_view.h"

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

class PrivateSDL :public Video_View
{
public:
	/**
	 * 初始化渲染窗口 线程安全.
	 *
	 * \param w 窗口宽度
	 * \param h 窗口高度
	 * \param fmt 绘制的像素格式，默认为RGBA
	 * \param win_id 窗口句柄，没有就重新创建一个
	 * \return 是否创建成功
	 */
    bool Init(int w, int h, Format fmt = RGBA, void* win_id = nullptr) override;

	/**
	 * 渲染图像 线程安全.
	 *
	 * \param data 渲染的二进制图像
	 * \param linesize 一行数组的字节数，若linesize<=0则自动计算大小
	 * \return 渲染是否成功
	 */
    bool Draw(const unsigned char* data, int linesize = 0) override;
	virtual bool Draw(
		const unsigned char* y, int y_pitch,
		const unsigned char* u, int u_pitch,
		const unsigned char* v, int v_pitch);


	/**
	 * 清理申请的资源，关闭窗口.
	 *
	 */
	void Close()override;

	/**
	 * 处理窗口退出事件.
	 *
	 * \return
	 */ 
	bool IsExit()override;

private:
	SDL_Window* win_ = nullptr;
	SDL_Renderer* render_ = nullptr;
	SDL_Texture* texture_ = nullptr;
};

