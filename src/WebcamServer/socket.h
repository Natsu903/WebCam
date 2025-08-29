#pragma once
#include <WinSock2.h>
#include <memory>

#pragma comment(lib, "ws2_32.lib")

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

	operator char* () const { return (char*)c_str(); }
	operator const char* () const { return c_str(); }
	operator BYTE* () const { return (BYTE*)c_str(); }
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
			*this+=buf; 
		}
		else
		{
			BBuffer tmp = buf;
			*this = tmp;
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
		*this += str;
		return *this;
	}
	BBuffer& operator<<(int data)
	{
		char s[16] = "";
		snprintf(s, sizeof(s), "%d", data);
		*this += s;
		return *this;
	}
	const BBuffer& operator>>(int data) const
	{
		data = atoi(c_str());
		return *this;
	}
	const BBuffer& operator>>(short data) const
	{
		data = (short)atoi(c_str());
		return *this;
	}
};

/**
* 封装套接字
 * .
 */
class Socket
{
public:
	// 构造函数，默认创建0:TCP套接字 1:UDP套接字  
	Socket(bool bIsType = true)
	{
		m_socket_ = INVALID_SOCKET;
		if (bIsType)
		{
			m_socket_ = socket(PF_INET, SOCK_STREAM, 0);
		}
        else
        {
            m_socket_ = socket(PF_INET, SOCK_DGRAM, 0);
        }
	}

    Socket(SOCKET socket)
	{
		m_socket_ = socket;
	}

	void Close()
	{
		if (m_socket_ != INVALID_SOCKET)
		{
			SOCKET tmp = m_socket_;
			m_socket_ = INVALID_SOCKET;
			closesocket(tmp);
		}
	}

	~Socket()
	{
		Close();
	}

	operator SOCKET()
	{
		return m_socket_;
	}
private:

	SOCKET m_socket_;
};

class EAddress
{
public:
	EAddress()
	{
		m_port_ = -1;
		memset(&m_addr_,0,sizeof(m_addr_));
		m_addr_.sin_family=AF_INET;
	}
	EAddress(const EAddress& addr)
	{
		m_ip_= addr.m_ip_;
		m_port_ = addr.m_port_;
		memcpy(&m_addr_,&addr.m_addr_,sizeof(sockaddr_in));
	}
	EAddress& operator=(const EAddress& addr)
	{
		if (this != &addr)
		{
            m_ip_ = addr.m_ip_;
            m_port_ = addr.m_port_;
            memcpy(&m_addr_,&addr.m_addr_,sizeof(sockaddr_in));
		}
		return *this;
	}
	~EAddress(){}
	void Update(const std::string& ip, short port)
	{
        m_ip_ = ip;
        m_port_ = port;
        m_addr_.sin_port = htons(port);
        m_addr_.sin_addr.s_addr = inet_addr(ip.c_str());
	}
	operator const sockaddr* () const
	{
        return (sockaddr*)&m_addr_;
	}

	operator sockaddr* ()
	{
		return (sockaddr*)&m_addr_;
	}

	operator sockaddr_in* ()
	{
        return &m_addr_;
	}
	int size()const{return sizeof(sockaddr_in);}
private:
	std::string m_ip_;
	short m_port_;
	sockaddr_in m_addr_;
};

/**
 * 智能指针封装的套接字，管理套接字生命周期
 * .
 */ 
class ESocket
{
public:
	ESocket(bool isTcp = true):m_socket_(new Socket(isTcp)),m_istcp_(isTcp){}

	ESocket(SOCKET sock, bool isTcp = true) :m_socket_(new Socket(isTcp)), m_istcp_(isTcp){}

	// 拷贝构造函数,深拷贝套接字对象  
	ESocket(const ESocket& socket):m_socket_(socket.m_socket_),m_istcp_(socket.m_istcp_) {}
	
	~ESocket()
	{
		m_socket_.reset();
	}

	//重载赋值运算符 
	ESocket& operator=(const ESocket& socket)
	{
		if (this != &socket)
		{
			m_socket_ = socket.m_socket_;
		}
		return *this;
	}

	// 隐式类型转换，将ESocket对象转为SOCKET类型 
	operator SOCKET()
	{
		return *m_socket_;
	}

	int Bind(const EAddress& addr)
	{
		if (m_socket_ == nullptr)
		{
			m_socket_.reset(new Socket(m_istcp_));
		}
		return ::bind(*m_socket_, addr, addr.size());
	}

	int Listen(int backlog = SOMAXCONN)
	{
		return ::listen(*m_socket_, backlog);
	}

	ESocket Accept(EAddress& addr)
	{
		int len=addr.size();
		SOCKET s = accept(*m_socket_, addr, &len);
		return ESocket(s,m_istcp_);
	}

	int Connect(const EAddress& addr)
	{
		return ::connect(*m_socket_, addr, addr.size());
	}

	int Recv(BBuffer& buffer)
	{
		return recv(*m_socket_, buffer,buffer.size(),0);
	}

    int Send(const BBuffer& buffer)
	{
		int index = 0;
		char* pData = buffer;
		while (index < (int)buffer.size())
        {
            int ret = send(*m_socket_, pData + index, buffer.size() - index, 0);
            if (ret < 0) return ret;
			if (ret == 0) break;
            index += ret;
        }
		return index;
	}

	void Close()
	{
		m_socket_.reset();
	}

private:
	std::shared_ptr<Socket> m_socket_;
	bool m_istcp_;
};

class SocketIniter
{
public:
	SocketIniter()
	{
		WSADATA wsa;
		WSAStartup(MAKEWORD(2, 2), &wsa);
	}
	~SocketIniter()
	{
		WSACleanup();
	}
};

