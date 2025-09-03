#pragma once
#include "BBuffer.h"
#include "socket.h"

class RTPHeader
{
public:
	/**
	 * DataSheet中默认左边是高位右边是低位
	 * 位域中先声明的是低位后声明的是高位
	 * 如果跨字节了，先声明的在前面，后声明的在后面，按字节排序.
	 */
	unsigned csrccount : 4;	//抄送计数: 4位
	unsigned extension : 1;	//扩展位: 1位
	unsigned padding : 1;	//填充位: 1位
	unsigned version : 2;	//版本号: 2位

	unsigned pytype : 7;	//负载类型: 7位
	unsigned mark : 1;		//标记位: 1位

	unsigned serial : 16;	//序列号: 16位
	unsigned timestamp;		//时间戳: 32位
	unsigned ssrc;			//同步信源: 32位
	unsigned csrc[15];		//特约信源: 15 * 32位

public:
	RTPHeader();
	RTPHeader(const RTPHeader& header);
    RTPHeader& operator=(const RTPHeader& header);
	//允许强制转换成BBuffer
	operator BBuffer();

};

class RTPFrame
{

public:
	RTPHeader m_head_;
	BBuffer m_pyload_;
	operator BBuffer();
};

class RTPHelper
{
public:
	RTPHelper():timestamp(0),m_udp(false)
	{
		m_udp.Bind(EAddress("0.0.0.0", (short)55000));
		m_file_=fopen("./out.bin", "wb+");
	}
	~RTPHelper()
	{
        fclose(m_file_);
	}

	int SendMediaFrame(RTPFrame& rtpframe, BBuffer& frame, const EAddress& client);

private:
	int GetFrameSepSize(BBuffer& frame);
	int SendFrame(const BBuffer& frame,const EAddress& client);

private:
	DWORD timestamp;
    ESocket m_udp;
	FILE* m_file_;
};

