#pragma once
#include <string>
#include "tools.h"
class CameraRecord:public BaseThread
{
public:
	void set_rtsp_url(std::string url) { rtsp_url_ = url; }
	void set_save_path(std::string s) { save_path_ = s; }
	void set_file_sec(long long s) { file_sec_ = s; }
private:
	void Main();
	std::string rtsp_url_;
	std::string save_path_;//存储根目录
	long long file_sec_ = 5;//多少秒创建一个新文件
};

