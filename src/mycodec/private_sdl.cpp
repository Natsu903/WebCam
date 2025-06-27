/*****************************************************************//**
 * \file   private_sdl.cpp
 * \brief  实现SDL实现的私有化，将视频渲染部分从主业务中剥离
 * 
 * \author Administrator
 * \date   April 2025
 *********************************************************************/

#include "private_sdl.h"
#include <iostream>
#include <SDL2/SDL.h>

#pragma comment(lib,"SDL2.lib")

/**
 * 初始化SDL，并确保有且只初始化一次.
 *
 * \return 成功返回true失败false
 */
static bool InitVideo()
{
    //static bool isfirst = true;
    //static std::mutex mux;
    //std::unique_lock<std::mutex> sdl_lock(mux);
    //if (!isfirst) return true;
    //isfirst = false;
    //if (!SDL_Init(SDL_INIT_VIDEO))
    //{
    //    std::cout << SDL_GetError() << std::endl;
    //    return false;
    //}
    //SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    //return true;
	static std::once_flag init_flag;
	std::call_once(init_flag, []() {
		if (SDL_Init(SDL_INIT_VIDEO) != 0) {
			std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
			return false;
		}
        //将纹理过滤设置为线性
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
		});
	return true;
}


bool PrivateSDL::Init(int w, int h, Format fmt, void* win_id)
{
    if (w <= 0 || h <= 0)return false;
    if(!InitVideo()) return false;
    std::unique_lock<std::mutex> sdl_lock(mtx_);
    width_ = w;
    height_ = h;
    format_ = fmt;

    //清除现有的render_，texture_，据SDL的文档，这些函数可以安全地传入NULL
	SDL_DestroyRenderer(render_);
	SDL_DestroyTexture(texture_);
    render_ = nullptr;
    texture_ = nullptr;

    //判断没有现成窗口时的处理
    if (!win_)
    {
        if (!win_id_)
        {
            win_ = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
        }
        else
        {
            win_ = SDL_CreateWindowFrom(win_id_);
        }
    }
	if (!win_)
	{
		std::cerr << SDL_GetError() << std::endl;
		return false;
	}
    //创建Renderer
    render_ = SDL_CreateRenderer(win_, -1, SDL_RENDERER_ACCELERATED);
    if (!render_)
	{
		std::cerr << SDL_GetError() << std::endl;
		return false;
	}
    //根据视频format更改参数sdl_fmt
    unsigned int sdl_fmt = SDL_PIXELFORMAT_RGBA8888;
    switch (fmt)
    {
    case Video_View::YUV420P:
        sdl_fmt = SDL_PIXELFORMAT_IYUV;
        break;
    case Video_View::ARGB:
        sdl_fmt = SDL_PIXELFORMAT_ARGB32;
        break;
    case Video_View::RGBA:
        sdl_fmt = SDL_PIXELFORMAT_RGBA32;
        break;
    case Video_View::BGRA:
        sdl_fmt = SDL_PIXELFORMAT_BGRA32;
        break;
	case Video_View::NV12:
		sdl_fmt = SDL_PIXELFORMAT_NV12;
		break;
    default:
        break;
    }
    //创建Texture
    texture_ = SDL_CreateTexture(render_,sdl_fmt,SDL_TEXTUREACCESS_STREAMING,w,h);
    if (!texture_)
    {
		std::cerr << SDL_GetError() << std::endl;
		return false;
    }
    return true;
}

bool PrivateSDL::Draw(const unsigned char* data, int linesize)
{
    if (!data)return false;
    std::unique_lock<std::mutex>sdl_clock(mtx_);
    if (!texture_ || !render_ || !win_ || width_ <= 0 || height_ <= 0) return false;
    if (linesize <= 0)
    {
        switch (format_)
        {
        case Video_View::RGBA:
        case Video_View::ARGB:
            linesize = width_ * 4;
            break;
        case Video_View::YUV420P:
            linesize = width_;
            break;
        default:
            break;
        }
    }
	if (linesize <= 0)return false;
	auto re = SDL_UpdateTexture(texture_, NULL, data, linesize);
	if (re!=0)
	{
		std::cout << SDL_GetError() << std::endl;
		return false;
	}
	SDL_RenderClear(render_);

	SDL_Rect rect;
	SDL_Rect* prect = nullptr;
	if (scale_w_ > 0)
	{
		rect.x = 0; rect.y = 0;
		rect.w = scale_w_;
		rect.h = scale_h_;
		prect = &rect;
	}
    re = SDL_RenderCopy(render_, texture_, NULL, prect);
	if (re != 0)
	{
		std::cout << SDL_GetError() << std::endl;
		return false;
	}
    SDL_RenderPresent(render_);
    return true;
}

bool PrivateSDL::Draw(const unsigned char* y, int y_pitch, const unsigned char* u, int u_pitch, const unsigned char* v, int v_pitch)
{
    if (!y || !u || !v)return false;
    std::unique_lock<std::mutex>sdl_lock(mtx_);
    if (!texture_ || !render_ || !win_ || width_ <= 0 || height_ <= 0) return false;
    auto re = SDL_UpdateYUVTexture(texture_, NULL, y, y_pitch, u, u_pitch, v, v_pitch);
    if (re != 0)
    {
        std::cout << SDL_GetError() << std::endl;
        return false;
    }
    SDL_RenderClear(render_);

	SDL_Rect rect;
	SDL_Rect* prect = nullptr;
	if (scale_w_ > 0)
	{
		rect.x = 0; rect.y = 0;
		rect.w = scale_w_;
		rect.h = scale_h_;
		prect = &rect;
	}
	re = SDL_RenderCopy(render_, texture_, NULL, prect);
	if (re != 0)
	{
		std::cout << SDL_GetError() << std::endl;
		return false;
	}
	SDL_RenderPresent(render_);
	return true;
}

void PrivateSDL::Close()
{
    std::unique_lock<std::mutex>sdl_lock(mtx_);
    if (render_)
    {
        SDL_DestroyRenderer(render_);
        render_ = nullptr;
    }
    if (texture_)
    {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
    if (win_)
    {
        SDL_DestroyWindow(win_);
        win_ = nullptr;
    }
}

bool PrivateSDL::IsExit()
{
    SDL_Event ev;
    SDL_WaitEventTimeout(&ev, 1);
    if (ev.type == SDL_QUIT) return true;
    return false;
}
