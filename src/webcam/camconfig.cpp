#include "camconfig.h"
#include <fstream>
#include <QDebug>

void CamConfig::Push(const CamData& data)
{
	std::unique_lock<std::mutex>lock(mux_);
	cams_.push_back(data);
}

CamData CamConfig::GetCam(int index)
{
	std::unique_lock<std::mutex>lock(mux_);
	if (index<0 || index>cams_.size())return CamData();
	return cams_[index];
}

bool CamConfig::SetCam(int index, const CamData& data)
{
	std::unique_lock<std::mutex>lock(mux_);
	if (index<0 || index>cams_.size())return false;
	cams_[index] = data;
	return true;
}

bool CamConfig::DeleteCam(int index)
{
	std::unique_lock<std::mutex>lock(mux_);
	if (index<0 || index>cams_.size())return false;
	cams_.erase(cams_.begin() + index);
	return true;
}

int CamConfig::GetCamCount()
{
	std::unique_lock<std::mutex>lock(mux_);
	return cams_.size();
}

bool CamConfig::Save(const char* path)
{
	if (!path)
	{
		qDebug() << "CamConfig::Save(const char* path)路径为空";
		return false;
	}
	std::ofstream ofs(path, std::ios::binary);
	if (!ofs)
	{
		qDebug() << "std::ofstream ofs(path, std::ios::binary);失败";
		return false;
	}
	for (auto& cam : cams_)
	{
		ofs.write((char*)&cam, sizeof(cam));
	}
	ofs.close();
	return true;
}

bool CamConfig::Load(const char* path)
{
	if (!path)
	{
		qDebug() << "CamConfig::Load(const char* path)路径为空";
		return false;
	}
	std::ifstream ifs(path, std::ios::binary);
	if (!ifs)
	{
		qDebug() << "std::ifstream ifs(path, std::ios::binary);失败";
		return false;
	}
	CamData camdata;
	std::unique_lock<std::mutex>lock(mux_);
	cams_.clear();
	for (;;)
	{
		ifs.read((char*)&camdata, sizeof(camdata));
		if (ifs.gcount() != sizeof(camdata))
		{
			ifs.close();
			return true;
		}
		cams_.push_back(camdata);
	}
	ifs.close();
	return true;
}
