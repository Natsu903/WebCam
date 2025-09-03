#include "rtspserver.h"
#include <rpc.h>
#include "bbuffer.h"

#pragma comment(lib, "rpcrt4.lib")


SocketIniter RTSPServer::m_initer_;

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
	m_pool_.Invoke();
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
		RTSPSession session(client);
		m_listsessions_.PushBack(session);
		m_pool_.DispatchWorker(ThreadWorker::ThreadWorker(this, (FUNCTYPE)&RTSPServer::ThreadSession));
	}
	return 0;
}

//接受数据请求，解析请求，应答请求
int RTSPServer::ThreadSession()
{
	RTSPSession session;
	if (m_listsessions_.PopFront(session))
	{
		int ret = session.PickRequestAndReply(RTSPServer::PlayCallBack,this);
		return ret;
	}
	return -1;
}

void RTSPServer::PlayCallBack(RTSPServer* thiz, RTSPSession& session)
{
	thiz->UdpWorker(session.GetClientUDPAddress());
}

void RTSPServer::UdpWorker(const EAddress& client)
{
	BBuffer frame = m_h264_.ReadOneFrame();
	RTPFrame rtp;
	while (frame.size() > 0)
	{
		m_helper_.SendMediaFrame(rtp, frame, client);
		frame=m_h264_.ReadOneFrame();
	}
}

RTSPReply::RTSPReply()
{
	m_method_ = -1;
}

RTSPReply::RTSPReply(const RTSPReply& protocol)
{
	m_method_ = protocol.m_method_;
	m_client_port_[0]= protocol.m_client_port_[0];
	m_client_port_[1]= protocol.m_client_port_[1];
	m_server_port_[0]= protocol.m_server_port_[0];
	m_server_port_[1]= protocol.m_server_port_[1];
	m_sdp_= protocol.m_sdp_;
	m_options_= protocol.m_options_;
	m_session_= protocol.m_session_;
	m_seq_= protocol.m_seq_;
}

RTSPReply& RTSPReply::operator=(const RTSPReply& protocol)
{
	if (this != &protocol)
	{
		m_method_ = protocol.m_method_;
		m_client_port_[0] = protocol.m_client_port_[0];
		m_client_port_[1] = protocol.m_client_port_[1];
		m_server_port_[0] = protocol.m_server_port_[0];
		m_server_port_[1] = protocol.m_server_port_[1];
		m_sdp_ = protocol.m_sdp_;
		m_options_ = protocol.m_options_;
		m_session_ = protocol.m_session_;
		m_seq_ = protocol.m_seq_;
	}
	return *this;
}

BBuffer RTSPReply::toBuffer()
{
	BBuffer result;
	result << "RTSP/1.0 200 OK\r\n" << "CSeq: " << m_seq_ << "\r\n";
	switch (m_method_)
	{
	case 0://OPTIONS
		result << "Public: OPTIONS, DESCRIBE，SETUP，PLAY, TEARDOWN\r\n" << "Server: RTSPServer\r\n\r\n";
		break;
    case 1://DESCRIBE
		result << "Content-Base: 127.0.0.1\r\n";
		result << "Content-Type: application/sdp\r\n";
		result << "Content-Length: " << m_sdp_.size() << "\r\n\r\n";
		result << (char*)m_sdp_;
		break;
	case 2://SETUP
        result << "Transport: RTP/AVP;unicast;client_port=" << m_client_port_[0] << "-" << m_client_port_[1];
		result << ";server_port=" << m_server_port_[0] << "-" << m_server_port_[1] << "\r\n";
		result << "Session:" << (char*)m_session_ << "\r\n\r\n";
		break;
	case 3://PLAY
		result << "Range: npt=0.000-\r\n";
        result << "Session:" << (char*)m_session_ << "\r\n\r\n";
		break;
	case 4://TEARDOWN
		result << "Session:" << (char*)m_session_ << "\r\n\r\n";
		break;
	}
	return result;
}


void RTSPReply::SetMethod(int method)
{
	m_method_=method;
}

void RTSPReply::SetOptions(const BBuffer& options)
{
	m_options_ = options;
}

void RTSPReply::SetSequence(const BBuffer& seq)
{
	m_seq_=seq;
}

void RTSPReply::SetSdp(const BBuffer& sdp)
{
	m_sdp_ = sdp;
}

void RTSPReply::SetClientPort(const BBuffer& port0, const BBuffer& port1)
{
	port0 >> m_client_port_[0];
	port1 >> m_client_port_[1];
}

void RTSPReply::SetServerPort(const BBuffer& port0, const BBuffer& port1)
{
	port0 >> m_server_port_[0];
	port1 >> m_server_port_[1];
}

void RTSPReply::SetSession(const BBuffer& session)
{
	m_session_ = session;
}

RTSPSession::RTSPSession(const ESocket& client) :m_client_(client)
{
	//保证生成唯一的session id
	UUID uuid;
	UuidCreate(&uuid);
	m_id_.resize(8);
	snprintf((char*)m_id_.c_str(), m_id_.size(), "%u%u", uuid.Data1, uuid.Data2);
	m_port_ = -1;
}

RTSPSession::RTSPSession(const RTSPSession& session)
{
	m_id_=session.m_id_;
	m_client_=session.m_client_;
	m_port_=session.m_port_;
}

RTSPSession::RTSPSession()
{
	m_port_ = -1;
	//保证生成唯一的session id
	UUID uuid;
	UuidCreate(&uuid);
	m_id_.resize(8);
	snprintf((char*)m_id_.c_str(), m_id_.size(), "%u%u", uuid.Data1, uuid.Data2);
}

int RTSPSession::PickRequestAndReply(RTSPPLAYCB cb, RTSPServer* thiz)
{
	int ret = -1;
	do 
	{
		BBuffer buffer = Pick();
		if (buffer.size() <= 0) return -1;
		RTSPRequest req = AnalyzeRequest(buffer);
		if (req.method() < 0)
		{
			ETool::Trace("Buffer:[%s]\r\n", (char*)buffer);
			return -2;
		}
		RTSPReply reply = Reply(req);
		ret = m_client_.Send(reply.toBuffer());
		if (req.method() == 2)
		{
			m_port_ = (short)atoi(req.port());
		}
		if (req.method() == 3)
		{
            cb(thiz,*this);
		}
	} while (ret>=0);
	if(ret<0) return ret;
	return 0;
}

EAddress RTSPSession::GetClientUDPAddress() const
{
	EAddress addr;
	int len = addr.size();
	getsockname(m_client_, addr,&len);
	addr.Fresh();
	addr = m_port_;
	return addr;
}

BBuffer RTSPSession::PickOneLine(BBuffer& buffer)
{
	if(buffer.size() <= 0) return BBuffer();
	BBuffer result,temp;
	int i = 0;
	for (; i<(int)buffer.size(); i++)
	{
		result+=buffer.at(i);
		if(buffer.at(i)=='\n') break;
	}
	temp = i + 1 + (char*)buffer;
	buffer = temp;
	return result;
}

BBuffer RTSPSession::Pick()
{
	BBuffer result;
	BBuffer buf(1);
	int ret = 1;
	while (ret > 0)
	{
		buf.Zero();//内存值置零不会改变大小
		ret = m_client_.Recv(buf);
		if (ret > 0)
		{
			result += buf;
			if (result.size() >= 4)
			{
				UINT val = *(UINT*)(result.size() - 4 + (char*)result);
				if (val == *(UINT*)"\r\n\r\n") break;
			}
		}
	}
	return result;
}

//解析请求
RTSPRequest RTSPSession::AnalyzeRequest(const BBuffer& buffer)
{
	ETool::Trace("<%s>\r\n", (char*)buffer);
	RTSPRequest request;
	if (buffer.size() <= 0) return request;
	BBuffer data = buffer;
	BBuffer line = PickOneLine(data);
	BBuffer method(32),url(1024),version(16),seq(64);
	if (sscanf(line, "%s %s %s\r\n", (char*)method, (char*)url, (char*)version) < 3)
	{
		ETool::Trace("Error at :[%s]\r\n",(char*)line);
		return request;
	}
	line=PickOneLine(data);
	if (sscanf(line, "CSeq: %s\r\n", (char*)seq) < 1)
	{
		ETool::Trace("Error at :[%s]\r\n", (char*)line);
		return request;
	}
	request.SetMethod(method);
    request.SetUrl(url);
	request.SetSeq(seq);
	if ((strcmp(method, "OPTIONS") == 0)|| (strcmp(method, "DESCRIBE") == 0))
	{
		//OPTIONS DESCRIBE
		return request;
	}
	else if(strcmp(method, "SETUP")==0)
	{
		//SETUP
		do
		{
			line = PickOneLine(data);
			if (strstr((const char*)line, "client_port=") == nullptr) continue;
			break;
		}while (line.size() > 0);
		int port[2] = { 0,0 };
		if (sscanf(line, "Transport: RTP/AVP;unicast;client_port=%d-%d\r\n",port,port+1) == 2)
		{
			request.SetClientPort(port);
			return request;
		}
	}
	else if((strcmp(method, "PLAY")==0)||(strcmp(method, "TEARDOWN") == 0))
	{
		//PLAY TEARDOWN
		line=PickOneLine(data);
		BBuffer session(64);
        if (sscanf(line, "Session: %s\r\n", (char*)session) == 1)
		{
			request.SetSession(session);
			return request;
		}
	}
	return request;
}

//应答请求
RTSPReply RTSPSession::Reply(const RTSPRequest& request)
{
	RTSPReply reply;
	reply.SetSequence(request.sequence());
	if (request.session().size() > 0)
	{
		reply.SetSession(request.session());
	}
	else
	{
		reply.SetSession(m_id_);
	}
	reply.SetMethod(request.method());
	switch (request.method())
	{
	case 0://OPTIONS
		reply.SetOptions("Public: OPTIONS, DESCRIBE，SETUP，PLAY, TEARDOWN\r\n");
		break;
	case 1://DESCRIBE
	{
		BBuffer sdp;
		sdp << "v=0\r\n";
		sdp << "o=- "<< (char*)m_id_ <<" 1 IN IP4 127.0.0.1\r\n";
		sdp << "t=0 0\r\n" << "a=control:*\r\n" << "m=video 0 RTP/AVP 96\r\n";
		sdp << "a=framerate:24\r\n";
		sdp << "a=rtpmap:96 H264/90000\r\n" << "a=control:track0\r\n";
		reply.SetSdp(sdp);
	}
		break;
	case 2://SETUP
		reply.SetClientPort(request.port(0), request.port(1));
        reply.SetServerPort("55000","55001");
		reply.SetSession(m_id_);
		break;
	case 3://PLAY
	case 4://TEARDOWN
		break;
	}
	return reply;
}

RTSPSession& RTSPSession::operator=(const RTSPSession& session)
{
	if (this != &session)
	{
		m_id_ = session.m_id_;
		m_client_ = session.m_client_;
		m_port_ = session.m_port_;
	}
	return *this;
}

RTSPRequest::RTSPRequest()
{
	m_method_ = -1;
}

RTSPRequest::RTSPRequest(const RTSPRequest& protocol)
{
	m_method_=protocol.m_method_;
	m_url_=protocol.m_url_;
	m_session_=protocol.m_session_;
	m_seq_=protocol.m_seq_;
	m_client_port_[0]=protocol.m_client_port_[0];
	m_client_port_[1]=protocol.m_client_port_[1];
}

void RTSPRequest::SetMethod(const BBuffer& method)
{
	if (strcmp(method, "OPTIONS") == 0) m_method_ = 0;
	else if (strcmp(method, "DESCRIBE") == 0) m_method_ = 1;
	else if (strcmp(method, "SETUP") == 0) m_method_ = 2;
	else if (strcmp(method, "PLAY") == 0) m_method_ = 3;
	else if (strcmp(method, "TEARDOWN") == 0) m_method_ = 4;
}

void RTSPRequest::SetUrl(const BBuffer& url)
{
	m_url_ = (char*)url;
}

void RTSPRequest::SetSeq(const BBuffer& seq)
{
	m_seq_ = (char*)seq;
}

void RTSPRequest::SetClientPort(int ports[])
{
	m_client_port_[0] << ports[0];
	m_client_port_[1] << ports[1];
}

void RTSPRequest::SetSession(const BBuffer& session)
{
	m_session_ = (char*)session;
}

RTSPRequest& RTSPRequest::operator=(const RTSPRequest& protocol)
{
	if (this != &protocol)
	{
		m_method_ = protocol.m_method_;
		m_url_ = protocol.m_url_;
		m_session_ = protocol.m_session_;
		m_seq_ = protocol.m_seq_;
		m_client_port_[0] = protocol.m_client_port_[0];
		m_client_port_[1] = protocol.m_client_port_[1];
	}
	return *this;
}
