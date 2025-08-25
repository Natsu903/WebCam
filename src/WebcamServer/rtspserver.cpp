#include "rtspserver.h"

RTSPServer::~RTSPServer()
{
	Stop();
}

int RTSPServer::Init(const std::string& ip/*="0.0.0.0"*/, short port /*= 554*/)
{
	m_addr_.Update(ip, port);
	m_socket_.Bind(m_addr_);
	m_socket_.Listen();
	
	return 0;
}

int RTSPServer::Invoke()
{
	m_threadMain_.Start();
	return 0;
}

void RTSPServer::Stop()
{
	m_socket_.Close();
	m_threadMain_.Stop();
	m_pool_.Stop();
}

int RTSPServer::threadWorker()
{
	EAddress client_addr;
	ESocket client= m_socket_.Accept(client_addr);
	if (client != INVALID_SOCKET)
	{
		m_clients_.PushBack(client);
		m_pool_.DispatchWorker(ThreadWorker::ThreadWorker(this, (FUNCTYPE)&RTSPServer::ThreadSession));
	}
	return 0;
}

RTSPRequest RTSPServer::AnalyseRequest(const std::string& data)
{
	return {};
}

RTSPReply RTSPServer::MakeReply(const RTSPRequest& request)
{
	return {};
}

//接受数据请求，解析请求，应答请求
int RTSPServer::ThreadSession()
{
	ESocket client;
	BBuffer buffer(1024 * 16);
	int len = client.Recv(buffer);
	if (len <= 0)
	{

		return -1;
	} 
	buffer.resize(len);
	RTSPRequest req = AnalyseRequest(buffer);
	RTSPReply rep = MakeReply(req);
	client.Send(rep.toBuffer());
	return 0;
}

BBuffer RTSPReply::toBuffer()
{
	return BBuffer();
}
