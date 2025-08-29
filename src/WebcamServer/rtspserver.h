#pragma once
#include "basethread.h"
#include "socket.h"
#include "basequeue.h"
#include <string>
#include <map>


class RTSPRequest
{
public:
	RTSPRequest();
	RTSPRequest(const RTSPRequest& protocol);
	RTSPRequest& operator=(const RTSPRequest& protocol);
	~RTSPRequest() { m_method_ = -1; }

	void SetMethod(const BBuffer& method);
	void SetUrl(const BBuffer& url);
	void SetSeq(const BBuffer& seq);
	void SetClientPort(int ports[]);
	void SetSession(const BBuffer& session);
	int method() const{return m_method_;}
	const BBuffer& url() const { return m_url_; }
	const BBuffer& session() const { return m_session_; }
	const BBuffer& sequence() const{return m_seq_;}
	const BBuffer& port(int index = 0) const { return index ? m_client_port_[1] : m_client_port_[0]; }

private:
	int m_method_;//-1 初始化 0 OPTIONS 1DESCRIBE 2SETUP 3PLAY 4TEARDOWN
	BBuffer m_url_;
	BBuffer m_session_;
	BBuffer m_seq_;
	BBuffer m_client_port_[2];
};

class RTSPReply
{
public:
	RTSPReply();
	RTSPReply(const RTSPReply& protocol);
	RTSPReply& operator=(const RTSPReply& protocol);
    ~RTSPReply(){}
	BBuffer toBuffer();
	void SetOptions(const BBuffer& options);
	void SetSequence(const BBuffer& seq);
	void SetSdp(const BBuffer& sdp);
    void SetClientPort(const BBuffer& port0, const BBuffer& port1);
    void SetServerPort(const BBuffer& port0, const BBuffer& port1);
    void SetSession(const BBuffer& session);
private:
	int m_method_;//0 OPTIONS 1DESCRIBE 2SETUP 3PLAY 4TEARDOWN
	short m_client_port_[2];
	short m_server_port_[2];
	BBuffer m_sdp_;
    BBuffer m_options_;
	BBuffer m_session_;
	BBuffer m_seq_;
};

class RTSPSession
{
public:
	RTSPSession();
	RTSPSession(const ESocket& client);
	RTSPSession(const RTSPSession& session);
	RTSPSession& operator=(const RTSPSession& session);
	~RTSPSession(){}

	//分析请求
	int PickRequestAndReply();

private:
	BBuffer PickOneLine(BBuffer& buffer);

	BBuffer Pick();

	RTSPRequest AnalyzeRequest(const BBuffer& buffer);

	RTSPReply Reply(const RTSPRequest& request);


private:
	std::string m_id_;
	ESocket m_client_;
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

	int ThreadSession();

private:
	static SocketIniter m_initer_;
	ESocket m_socket_;
	EAddress m_addr_;
	//0未初始化 1初始化成功 2运行中 3停止
	int m_status_;
	BaseThread m_threadMain_;
	BaseThreadPool m_pool_;
	//std::map<std::string, std::string> m_mapSessions_;
	BaseQueue<RTSPSession> m_listsessions_;
};

