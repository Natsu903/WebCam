#include "vlcdecode.h"

#pragma comment(lib, "libvlc.lib")
#pragma comment(lib, "libvlccore.lib")

void VLCDecode::Init(int argc, const char* const* argv)
{
	instance_ = libvlc_new(argc, argv);
	if (!instance_) {
		LOGERROR("libvlc_new failed");
		return;
	}
}

void VLCDecode::SetWidthAndHeight(int width, int height)
{
	width_ = width;
    height_ = height;
}

void VLCDecode::Play(const char* url, void* hwnd)
{
	if (!instance_) {
		return;
	}

	media_ = libvlc_media_new_location(instance_, url ? url : "rtsp://127.0.0.1:554");
	if (!media_) {
		return;
	}

	mp_ = libvlc_media_player_new_from_media(media_);
	if (!mp_) {
		libvlc_media_release(media_);
		return;
	}

	libvlc_media_release(media_);

	if (hwnd) {
		libvlc_media_player_set_hwnd(mp_, hwnd);
	}

	int result = libvlc_media_player_play(mp_);
	if (result != 0) {
		// 播放失败处理
		libvlc_media_player_release(mp_);
		mp_ = nullptr;
		return;
	}

	MSleep(5000);

	auto length = libvlc_media_player_get_length(mp_);
	auto width = libvlc_video_get_width(mp_);
	auto height = libvlc_video_get_height(mp_);

	// 可以添加对width和height有效性的检查
	if (width == 0 || height == 0) {
		LOGERROR("视频尺寸无效")
	}

}

void VLCDecode::Stop()
{
	if (mp_) {
		libvlc_media_player_stop(mp_);
		libvlc_media_player_release(mp_);
		mp_ = nullptr;
	}

	if (instance_) {
		libvlc_release(instance_);
		instance_ = nullptr;
	}
}