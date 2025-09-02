#pragma once
#include <string>

typedef unsigned char BYTE;

class BBuffer :public std::string
{
public:
	BBuffer(const char* str)
	{
		resize(strlen(str));
		memcpy((void*)c_str(), str, size());
	}
	BBuffer(size_t size = 0) :std::string()
	{
		if (size > 0)
		{
			resize(size);
			memset(*this, 0, this->size());
		}
	}
	BBuffer(void* buffer, size_t size) :std::string()
	{
		resize(size);
		memcpy((void*)c_str(), buffer, size);
	}
	~BBuffer()
	{
		std::string::~basic_string();
	}

	//将BBuffer对象转换为char*指针
	operator char* () const { return (char*)c_str(); }
	//将BBuffer对象转换为const char*指针
	operator const char* () const { return c_str(); }
	//将BBuffer对象转换为unsigned char*指针
	operator BYTE* () const { return (BYTE*)c_str(); }
	//将BBuffer对象转换为void*指针
	operator void* () const { return (void*)c_str(); }
	void Update(const char* buffer, size_t size)
	{
		resize(size);
		memcpy((void*)c_str(), buffer, size);
	}

	void Zero()
	{
		if (size() > 0)
			memset((void*)c_str(), 0, size());
	}

	BBuffer& operator<<(const BBuffer& buf)
	{
		if (this != buf)
		{
			*this += buf;
		}
		else {
			BBuffer tmp = buf;
			*this += tmp;
		}
		return *this;
	}
	BBuffer& operator<<(const std::string& str)
	{
		*this += str;
		return *this;
	}
	BBuffer& operator<<(const char* str)
	{
		*this += BBuffer(str);
		return *this;
	}
	BBuffer& operator<<(int data)
	{
		char s[16] = "";
		snprintf(s, sizeof(s), "%d", data);
		*this += s;
		return *this;
	}
	const BBuffer& operator>>(int& data) const
	{
		data = atoi(c_str());
		return *this;
	}
	const BBuffer& operator>>(short& data) const
	{
		data = (short)atoi(c_str());
		return *this;
	}
};