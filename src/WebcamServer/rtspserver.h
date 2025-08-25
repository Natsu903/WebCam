#pragma once
#include "basethread.h"
#include "socket.h"
#include "basequeue.h"
#include <string>
#include <map>


class RTSPRequest
{
public:
	RTSPRequest(){}
	RTSPRequest(const RTSPRequest& protocol){}
	RTSPRequest& operator=(const RTSPRequest& protocol){}
    ~RTSPRequest(){}

private:
	int m_method_;//0 OPTIONS 1DESCRIBE 2SETUP 3PLAY 4TEARDOWN
};

class RTSPReply
{
public:
	RTSPReply(){}
	RTSPReply(const RTSPReply& protocol){}
	RTSPReply& operator=(const RTSPReply& protocol){}
    ~RTSPReply(){}
	BBuffer toBuffer();
private:
	int m_method_;//0 OPTIONS 1DESCRIBE 2SETUP 3PLAY 4TEARDOWN
};

class RTSPSession
{
public:
	RTSPSession(){}
    RTSPSession(const RTSPSession& protocol){}
    RTSPSession& operator=(const RTSPSession& protocol){}
	~RTSPSession(){}
};


class RTSPServer:public ThreadFuncBase
{
public:
	RTSPServer():m_socket_(true), m_status_(0), m_pool_(10)
	{
		m_threadMain_.UpdateWorker(::ThreadWorker(this, (FUNCTYPE)&RTSPServer::threadWorker));
	}
	~RTSPServer();

	//初始化服务器
	int Init(const std::string& ip="0.0.0.0", short port = 554);

	int Invoke();

	void Stop();

protected:
	//返回0表示继续运行，返回负数表示退出线程，返回其他警告
	int threadWorker();
	//分析请求
	RTSPRequest AnalyseRequest(const std::string& data);

	//应答请求
	RTSPReply MakeReply(const RTSPRequest& request);

	int ThreadSession();

private:
	ESocket m_socket_;
	EAddress m_addr_;
	//0未初始化 1初始化成功 2运行中 3停止
	int m_status_;
	BaseThread m_threadMain_;
	BaseThreadPool m_pool_;
	std::map<std::string, std::string> m_mapSessions_;
	static SocketIniter m_initer_;
	BaseQueue<ESocket> m_clients_;
};

