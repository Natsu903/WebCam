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

long MediaFile::FindH264Head(int& headsize)
{
	while (!feof(m_file_)) {
		char c = 0x7F;
		while (!feof(m_file_)) {//feof = file end of file
			c = fgetc(m_file_);
			if (c == 0) break;
		}
		if (!feof(m_file_)) {
			c = fgetc(m_file_);
			if (c == 0) {
				c = fgetc(m_file_);
				if (c == 1) {//找到了一个头
					headsize = 3;
					return ftell(m_file_) - 3;
				}
				else if (c == 0) {
					c = fgetc(m_file_);
					if (c == 1) {//找到了一个头
						headsize = 4;
						return ftell(m_file_) - 4;
					}
				}
			}
		}
	}
	return -1;
}

BBuffer MediaFile::ReadH264Frame()
{
	if (m_file_) {
		int headsize = 0;
		long off = FindH264Head(headsize);
		if (off == -1) return BBuffer();
		fseek(m_file_, off + headsize, SEEK_SET);
		long tail = FindH264Head(headsize);
		if (tail == -1) tail = m_size_;
		long size = tail - off;
		fseek(m_file_, off, SEEK_SET);
		BBuffer result(size);
		fread(result, 1, size, m_file_);
		return result;
	}
	return BBuffer();
}