#include "rtphelper.h"
#include <Windows.h>

#define RTP_MAX_SIZE 1300

int RTPHelper::SendMediaFrame(RTPFrame& rtpframe,BBuffer& frame, const EAddress& client)
{
	size_t frame_size = frame.size();
	int sepsize = GetFrameSepSize(frame);
	frame_size -= sepsize;
	BYTE* pFrame = sepsize+(BYTE*)frame;
	//传输的文件大于一个包的时，需要分片传输
	if (frame_size > RTP_MAX_SIZE)
	{
		BYTE nalu = pFrame[0] & 0x1F;
		// 计算分片数量和剩余字节
		size_t rest = frame_size % RTP_MAX_SIZE;	// 最后一个分片的大小
		size_t count = frame_size / RTP_MAX_SIZE;	// 完整分片的数量
		// 循环处理每个完整分片
		for (size_t i = 0; i < count; i++)
		{
			rtpframe.m_pyload_.resize(RTP_MAX_SIZE);
			// 设置FU-A分片的NAL头部 (0x60表示STAP-A包类型，28表示FU-A分片单元)
			((BYTE*)rtpframe.m_pyload_)[0] = 0x60 | 28;
			// 提取原始NAL单元类型并设置FU header
			((BYTE*)rtpframe.m_pyload_)[1] = nalu;//中间
			// 如果是第一个分片，设置起始位(0x80)
			if(i==0)
				((BYTE*)rtpframe.m_pyload_)[1] = 0x80 | ((BYTE*)rtpframe.m_pyload_)[1];//开始
			// 如果是最后一个分片且没有剩余数据，设置结束位(0x40)
			else if((rest==0)&&(i==count-1))
				((BYTE*)rtpframe.m_pyload_)[1] = 0x40 | ((BYTE*)rtpframe.m_pyload_)[1];//结束
			memcpy(2 + (BYTE*)rtpframe.m_pyload_, pFrame+RTP_MAX_SIZE*i, RTP_MAX_SIZE);
			SendFrame(rtpframe, client);
			rtpframe.m_head_.serial++;
		}
		// 如果有剩余数据，处理最后一个不完整的分片
		if (rest > 0)
		{
			// 设置结束位(0x40)表示这是最后一个分片
			((BYTE*)rtpframe.m_pyload_)[1] = 0x40 | ((BYTE*)rtpframe.m_pyload_)[1];//结束
			SendFrame(rtpframe, client);
			rtpframe.m_head_.serial++;
		}
	}
	else
	{
		rtpframe.m_pyload_.resize(frame.size() - sepsize);
        memcpy(rtpframe.m_pyload_, frame, frame.size() - sepsize);
		SendFrame(rtpframe, client);
		rtpframe.m_head_.serial++;
	}
	rtpframe.m_head_.timestamp += 90000 / 24;
	Sleep(1000 / 30);
	return 0;
}

int RTPHelper::GetFrameSepSize(BBuffer& frame)
{
	BYTE buf[] = { 0,0,0,1 };
	if(memcmp(frame,buf,4)==0) return 4;
	return 3;
}

int RTPHelper::SendFrame(const BBuffer& frame, const EAddress& client)
{
	int ret = sendto(m_udp, frame, frame.size(), 0, client, client.size());;
	return ret;
}

RTPHeader::RTPHeader()
{
	csrccount = 0;
	extension = 0;
	padding = 0;
	version = 2;
	pytype = 96;
	mark = 0;
	serial = 0;
	timestamp = 0;
	ssrc = 0x98765432;
	memset(csrc,0,sizeof(csrc));
}

RTPHeader::RTPHeader(const RTPHeader& header)
{
	memset(csrc,0,sizeof(csrc));
	int size = 14 + 4 * csrccount;
	memcpy(this, &header, size);
}

RTPHeader& RTPHeader::operator=(const RTPHeader& header)
{
	if (this != &header)
	{
		int size = 14 + 4 * csrccount;
		memcpy(this, &header, size);
	}
	return *this;
}

RTPHeader::operator BBuffer()
{
	RTPHeader header = *this;
	header.serial=htons(header.serial);
	header.timestamp = htonl(header.timestamp);
    header.ssrc = htonl(header.ssrc);
	int size = 14+4*csrccount;
	BBuffer result(size);
	memcpy(result,&header,size);
	return result;
}

RTPFrame::operator BBuffer()
{
	BBuffer result;
	result += (BBuffer)m_head_;
	result += m_pyload_;
	return result;
}
