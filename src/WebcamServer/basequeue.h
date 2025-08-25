#pragma once
#include <atomic>
#include <list>
#include "basethread.h"

//线程安全的队列
template<class T>
class BaseQueue
{
public:
    enum
    {
        BNone,
        BPush,
        BPop,
        BSize,
        BClear
    };
    //投递信息结构体
    typedef struct IocpParam
    {
        size_t nOperator;//操作
        T Data;//数据
        HANDLE hEvent;//pop操作需要的 等待线程填写数据完成唤醒
        IocpParam(int op, const T& data, HANDLE hEve = nullptr)
        {
            nOperator = op;
            Data = data;
            hEvent = hEve;
        }
        IocpParam() 
        {
            nOperator = BNone;
        }
    }PPARAM;//Post Parameter 用于投递信息的结构体
public:
	BaseQueue() 
    {
        m_lock_ = false;//队列是否在析构
		//创建一个完全端口映射
        m_hCompeletionPort_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
		m_hThread_ = INVALID_HANDLE_VALUE;
		if (m_hCompeletionPort_ != nullptr)
        {
			//开启一个线程 处理完全端口事件
            m_hThread_ = (HANDLE)_beginthread(&BaseQueue<T>::threadEntry,0, this);
		}
	}

	virtual ~BaseQueue() 
    {
		if (m_lock_)return;//队列已经析构了
        m_lock_ = true;
		PostQueuedCompletionStatus(m_hCompeletionPort_, 0, NULL, NULL);//发送一个结束信号 线程收到自动结束
		WaitForSingleObject(m_hThread_, INFINITE);//回收线程

		//回收完全端口 虽然线程中也会回收 防止完全端口创建了 线程没启动成功 
		if (m_hCompeletionPort_ != nullptr) 
		{
			HANDLE hTemp = m_hCompeletionPort_;
            m_hCompeletionPort_ = nullptr;
			CloseHandle(hTemp);
		}
	}

	bool PushBack(const T& data) 
	{
		IocpParam* pParam = new IocpParam(BPush, data);
		if (m_lock_) //已经析构了
		{
			delete pParam;
			return false;
		}
		//发送入队信号
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort_, sizeof(PPARAM), (ULONG_PTR)pParam, nullptr);
		if (ret == false)delete pParam;
		return ret;
	}

	virtual bool PopFront(T& data) 
	{
		HANDLE hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);//创建事件，阻塞等待线程操作完之后唤醒
		IocpParam Param(BPop, data, hEvent);
		if (m_lock_) //已经析构
		{
			if (hEvent)CloseHandle(hEvent);
			return false;
		}
		//发送出队信号
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort_, sizeof(PPARAM), (ULONG_PTR)&Param, nullptr);
		if (ret == false) 
		{
			CloseHandle(hEvent);
			return false;
		}
		//等待线程完成操作
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		//传回出队数据
		if (ret) 
		{
			data = Param.Data;
		}
		return ret;
	}

	//查看队列大小
	size_t Size() 
	{
		HANDLE hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		IocpParam Param(BSize, T(), hEvent);
		if (m_lock_) 
		{
			if (hEvent)CloseHandle(hEvent);
			return -1;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort_, sizeof(PPARAM), (ULONG_PTR)&Param, nullptr);
		if (ret == false) 
		{
			CloseHandle(hEvent);
			return -1;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		if (ret) 
		{
			return Param.nOperator;
		}
		return -1;
	}

	//清空队列
	bool Clear() 
	{
		if (m_lock_)return false;
		IocpParam* pParam = new IocpParam(BClear, T());
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort_, sizeof(PPARAM), (ULONG_PTR)pParam, nullptr);
		if (ret == false)delete pParam;
		return ret;
	}

protected:
	//线程入口
    static void threadEntry(void* arg)
    { 
		BaseQueue<T> * thiz = (BaseQueue<T>*)arg;
		thiz->threadMain();
		_endthread();
    }

	//操作处理函数
	virtual void DealParam(PPARAM* pParam) 
	{
		switch (pParam->nOperator)
		{
		case BPush://入队
			m_lstData_.push_back(pParam->Data);
			delete pParam;
			break;
		case BPop://出队
			if (m_lstData_.size() > 0) 
			{
				pParam->Data = m_lstData_.front();
				m_lstData_.pop_front();
			}
			//唤醒阻塞主线程
			if (pParam->hEvent != NULL)SetEvent(pParam->hEvent);
			break;
		case BSize://大小
			pParam->nOperator = m_lstData_.size();
			if (pParam->hEvent != NULL)
				SetEvent(pParam->hEvent);
			break;
		case BClear://清空队列
			m_lstData_.clear();
			delete pParam;
			break;
		default:
			OutputDebugStringA("unknown operator!\r\n");
			break;
		}
	}
	
	//监听线程(循环内核上报事件)
	virtual void threadMain() 
	{
		DWORD dwTransferred = 0;//内核填写了多少数据
		PPARAM* pParam = nullptr;
		ULONG_PTR CompletionKey = 0;
		OVERLAPPED* pOverlapped = nullptr;//重叠结构
		while (GetQueuedCompletionStatus(m_hCompeletionPort_,&dwTransferred,&CompletionKey,&pOverlapped, INFINITE))
		{
			if ((dwTransferred == 0) || (CompletionKey == NULL)) //接收到退出信号
			{
				printf("thread is prepare to exit!\r\n");
				break;
			}
			//接收到事件 处理
			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);
		}
		//防止析构通知结束时，内核中还有残留数据
		while (GetQueuedCompletionStatus(m_hCompeletionPort_,&dwTransferred,&CompletionKey,&pOverlapped, 0))
		{
			if ((dwTransferred == 0) || (CompletionKey == NULL)) {
				printf("thread is prepare to exit!\r\n");
				continue;
			}
			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);
		}
		//关闭完全端口
		HANDLE hTemp = m_hCompeletionPort_;
		m_hCompeletionPort_ = NULL;
		CloseHandle(hTemp);
	}
protected:
	std::list<T> m_lstData_;//队列
	HANDLE m_hCompeletionPort_;
	HANDLE m_hThread_;
	std::atomic<bool> m_lock_;//队列正在析构
};

template<class T>
class BaseSendQueue :public BaseQueue<T>, public ThreadFuncBase
{
public:
	typedef int (ThreadFuncBase::* BCALLBACK)(T& data);
	BaseSendQueue(ThreadFuncBase* obj, BCALLBACK callback)
		:BaseQueue<T>(), m_base_(obj), m_callback_(callback)
	{
		m_thread_.Start();//启动线程
		m_thread_.UpdateWorker(::ThreadWorker(this, (FUNCTYPE)&BaseSendQueue<T>::threadTick));//注册线程函数
	}
	virtual ~BaseSendQueue() 
	{
		m_base_ = nullptr;
		m_callback_ = NULL;
        m_thread_.Stop();
	}
protected:
	virtual bool PopFront(T& data)
	{
		return false;
	};
	//出队
	bool PopFront() 
	{
		typename BaseQueue<T>::IocpParam* Param = new typename BaseQueue<T>::IocpParam(BaseQueue<T>::BPop, T());
		if (BaseQueue<T>::m_lock_) //已经析构了
		{
			delete Param;
			return false;
		}
		//发送出队信号
		bool ret = PostQueuedCompletionStatus(BaseQueue<T>::m_hCompeletionPort_, sizeof(*Param), (ULONG_PTR)&Param, NULL);
		if (ret == false) 
		{
			delete Param;
			return false;
		}
		return ret;
	}
	int threadTick() 
	{
		if(WaitForSingleObject(BaseQueue<T>::m_hThread_, 0) !=WAIT_TIMEOUT)
            return 0;
		if (BaseQueue<T>::m_lstData_.size() > 0) {
			PopFront();
		}
		return 0;
	}
	//事件处理函数(重载父类)
	virtual void DealParam(typename BaseQueue<T>::PPARAM* pParam) 
	{
		switch (pParam->nOperator)
		{
		case BaseQueue<T>::BPush:
			BaseQueue<T>::m_lstData_.push_back(pParam->Data);
			delete pParam;
			break;
		case BaseQueue<T>::BPop:
			if (BaseQueue<T>::m_lstData_.size() > 0) {
				pParam->Data = BaseQueue<T>::m_lstData_.front();
				if ((m_base_->*m_callback_)(pParam->Data) == 0)
					BaseQueue<T>::m_lstData_.pop_front();
			}
			delete pParam;
			break;
		case BaseQueue<T>::BSize:
			pParam->nOperator = BaseQueue<T>::m_lstData_.size();
			if (pParam->hEvent != NULL)
				SetEvent(pParam->hEvent);
			break;
		case BaseQueue<T>::BClear:
			BaseQueue<T>::m_lstData_.clear();
			delete pParam;
			break;
		default:
			OutputDebugStringA("unknown operator!\r\n");
			break;
		}
	}
private:
	ThreadFuncBase* m_base_;//线程函数对象
	BCALLBACK m_callback_;//回调 int(*)(T& data)
	BaseThread m_thread_;//线程对象
};

typedef BaseSendQueue<std::vector<char>>::BCALLBACK  SENDCALLBACK;


