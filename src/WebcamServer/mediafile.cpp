#include "mediafile.h"

MediaFile::MediaFile():m_file_(nullptr),m_type_(-1)
{

}

MediaFile::~MediaFile()
{
	Close();
}

int MediaFile::Open(const BBuffer& filepath, int nType/*=96*/)
{
	m_file_ = fopen(filepath, "rb");
	if (m_file_ == nullptr)
	{
		return -1;
	}
	m_type_ = nType;
	fseek(m_file_, 0, SEEK_END);
	m_size_ = ftell(m_file_);
	if (m_size_ < 0) {
		Close();
		return -1;
	}
	Reset();
	return 0;
}

BBuffer MediaFile::ReadOneFrame()
{
	switch (m_type_)
	{
	case 96:
		return ReadH264Frame();
	}
	return BBuffer();
}

void MediaFile::Close()
{
	m_type_ = -1;
	if (m_file_ != nullptr)
	{
		FILE *file=m_file_;
		m_file_ = nullptr;
		fclose(file);
	}
}

void MediaFile::Reset()
{
	if (m_file_)
	{
		fseek(m_file_, 0, SEEK_SET);
	}
}

long MediaFile::FindH264Head()
{
	unsigned char buffer[4];

	// 重置文件指针到开始位置
	fseek(m_file_, 0, SEEK_SET);

	while (!feof(m_file_)) {
		// 读取一个字节
		int byte = fgetc(m_file_);
		if (byte == EOF) {
			break;
		}

		// 检查是否为0x00
		if ((unsigned char)byte == 0x00) {
			// 记录当前位置
			long current_pos = ftell(m_file_);

			// 尝试读取后续3个字节
			size_t read_count = fread(buffer, 1, 3, m_file_);
			if (read_count >= 2) {
				// 检查是否匹配H.264起始码模式
				if (buffer[0] == 0x00 && buffer[1] == 0x01) {
					// 找到3字节起始码: 0x000001
					return current_pos - 1;  // 指向第一个0x00的位置
				}
				else if (read_count >= 3 && buffer[0] == 0x00 && buffer[1] == 0x00 && buffer[2] == 0x01) {
					// 找到4字节起始码: 0x00000001
					return current_pos - 1;  // 指向第一个0x00的位置
				}
			}

			// 如果没有找到匹配，回退文件指针到下一个位置继续查找
			fseek(m_file_, current_pos, SEEK_SET);
		}
	}

	return -1;  // 未找到H.264头部
}

BBuffer MediaFile::ReadH264Frame()
{
	if (!m_file_) {
		return BBuffer();
	}

	long off = FindH264Head();
	if (off == -1) {
		return BBuffer();
	}

	// 保存当前位置并查找下一帧
	long current_pos = ftell(m_file_);
	fseek(m_file_, off + (ftell(m_file_) - current_pos), SEEK_SET);

	long tail = FindH264Head();
	if (tail == -1) {
		tail = m_size_;
	}

	long size = tail - off;
	if (size <= 0) {
		return BBuffer();
	}

	fseek(m_file_, off, SEEK_SET);
	BBuffer result(size);
	fread(result, 1, size, m_file_);

	return result;
}