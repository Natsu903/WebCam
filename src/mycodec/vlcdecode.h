#pragma once
#include <vlc/vlc.h>
#include "tools.h"

class WEBCAM_API VLCDecode
{
public:
    VLCDecode(){}
    ~VLCDecode()
    {
        Stop();
    }

    //初始化VLC组件
    void Init(int argc=0, const char* const* argv=nullptr);

    void SetWidthAndHeight(int width,int height);

    //播放url链接
    void Play(const char* url,void* hwnd=nullptr);

    //停止播放
    void Stop();

private:
    libvlc_instance_t* instance_ =nullptr;
    libvlc_media_t* media_=nullptr;
    libvlc_media_player_t* mp_ = nullptr;
    libvlc_time_t length_ = 0;
    int width_ = 0;
    int height_ = 0;
};

