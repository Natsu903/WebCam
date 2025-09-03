#pragma once
#include "bbuffer.h"

class MediaFile
{
public:
	MediaFile();
	~MediaFile();

	int Open(const BBuffer& filepath,int nType=96);
	//如果 Buffer的size为0，则表示没有帧了
	BBuffer ReadOneFrame();

	void Close();
	//重置后ReadOneFrame又会有值返回
	void Reset();

private:
	long FindH264Head(int& headsize);
	BBuffer ReadH264Frame();
private:
	long m_size_;
	FILE *m_file_;
	BBuffer *m_filepath_;
	//96 H264
	int m_type_;
};

