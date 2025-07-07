#pragma once
#include <vector>
#include <mutex>

struct CamData
{
	char name[1024] = { 0 };		//名称
	char url[4096] = { 0 };			//主码流
	char sub_url[4096] = { 0 };		//辅码流
	char save_path[4096] = { 0 };	//存放目录
};

class CamConfig
{
public:
	static CamConfig* Instance()
	{
		static CamConfig cc;
		return &cc;
	}

	//插入 线程安全
	void Push(const CamData& data);

	//获取摄像机
	CamData GetCam(int index);

	//修改摄像机数据
	bool SetCam(int index, const CamData& data);

	//删除摄像机数据
	bool DeleteCam(int index);

	//获取摄像机数量,失败返回0
	int GetCamCount();

	//数据持久化
	bool Save(const char* path);

	//读取本地化数据
	bool Load(const char* path);
private:
	CamConfig() {};
	std::vector<CamData> cams_;
	std::mutex mux_;
};

