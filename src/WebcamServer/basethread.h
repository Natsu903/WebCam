#pragma once
#include <windows.h>
#include <process.h>
#include <atomic>
#include <vector>
#include <mutex>

class ETool
{
public:
	static void Trace(const char* format, ...)
	{
		va_list ap;
		va_start(ap, format);
		std::string sBuffer;
		sBuffer.resize(1024 * 10);
		int result = _vsnprintf((char*)sBuffer.data(), sBuffer.size() - 1, format, ap);
		if (result >= 0) {
			sBuffer[result] = '\0'; // 确保字符串结尾
			OutputDebugStringA(sBuffer.c_str());
		}
		va_end(ap);
	}
};

class ThreadFuncBase {};
typedef int (ThreadFuncBase::* FUNCTYPE)();
class ThreadWorker
{
public:
	ThreadWorker() :thiz_(nullptr), func_(nullptr){}
	ThreadWorker(void* obj, FUNCTYPE f) :thiz_((ThreadFuncBase*)obj), func_(f) {}
	//复制构造函数
	ThreadWorker(const ThreadWorker& worker)
	{
		thiz_ = worker.thiz_;
		func_ = worker.func_;
	}
	//等于号重载
	ThreadWorker& operator=(const ThreadWorker& worker)
	{
		if (this != &worker)
		{
			thiz_ = worker.thiz_;
			func_ = worker.func_;
		}
		return *this;
	}
	//函数调用运算符重载
	int operator()()
	{
		if (IsValid())
		{
			return (thiz_->*func_)();
		}
		return -1;
	}

	bool IsValid() const
	{
		return thiz_ != nullptr && func_ != nullptr;
	}

private:
	//线程对象指针
	ThreadFuncBase* thiz_;
	//成员函数指针
	FUNCTYPE func_;
};

class BaseThread
{
public:
	BaseThread()
	{
		m_hThread_ = nullptr;
		m_bStatus_ = false;
	}
	~BaseThread()
	{
		Stop();
	}

	//true表示启动成功
	bool Start()
	{
		m_bStatus_ = true;
		m_hThread_ = (HANDLE)_beginthread(&BaseThread::ThreadEntry, 0, this);
		if (!IsValid())
		{
			m_bStatus_ = false;
		}
		return m_bStatus_;
	}

	/**
	 * .
	 * 如果句柄不存在，WaitForSingleObject返回错误
	 * 如果结束了，WaitForSingleObject返回WAIT_ABANDONED
	 * 如果刚好在这一瞬间结束，WaitForSingleObject返回WAIT_OBJECT_0
	 * 只有WAIT_TIMEOUT表示线程没有结束
	 * \return true表示线程有效，false表示线程无效
	 */
	bool IsValid()
	{
		if (m_hThread_ == nullptr || (m_hThread_ == INVALID_HANDLE_VALUE)) return false;
		return WaitForSingleObject(m_hThread_, 0) == WAIT_TIMEOUT;
	}

	bool Stop()
	{
		if (m_bStatus_ == false)return true;
		m_bStatus_ = false;
		DWORD ret = WaitForSingleObject(m_hThread_, 1000);
		if (ret==WAIT_TIMEOUT)
		{
			TerminateThread(m_hThread_, -1);
		}
		UpdateWorker();
		return ret==WAIT_OBJECT_0;
	}

	void UpdateWorker(const ::ThreadWorker& worker = ::ThreadWorker())
	{
		if (m_worker_.load() != nullptr && (m_worker_.load() != &worker))
		{
			::ThreadWorker* pWorker = m_worker_.load();
			ETool::Trace("delete pWorker = %08X m_worker = %08X\r\n", pWorker, m_worker_.load());
			m_worker_.store(nullptr);
			delete pWorker;
		}
		if (m_worker_.load() == &worker)return;
		if (!worker.IsValid())
		{
			m_worker_.store(nullptr);
			return;
		}
		::ThreadWorker* pWorker = new ::ThreadWorker(worker);
		ETool::Trace("new pWorker = %08X m_worker = %08X\r\n", pWorker, m_worker_.load());
        m_worker_.store(pWorker);
	}

	//是否空闲 true 空闲 false 非空闲
	bool IsIdel()
	{
		if (m_worker_.load() == nullptr) return true;
		return !m_worker_.load()->IsValid();
	}



private:
	void ThreadWorker()
	{
		while (m_bStatus_)
		{
			if (m_worker_.load() == nullptr)
			{
				Sleep(1);
				continue;
			}
			::ThreadWorker worker = *m_worker_.load();
			if (worker.IsValid())
			{
				if (WaitForSingleObject(m_hThread_, 0) == WAIT_TIMEOUT)
				{
					int ret = worker();
					if (ret != 0)
					{
						ETool::Trace("Thread found warning code %d\r\n", ret);
					}
					if (ret < 0)
					{
						::ThreadWorker* pWorker = m_worker_.load();
						m_worker_.store(nullptr);
						delete pWorker;
					}
				}
			}
			else
			{
				Sleep(1);
			}
		}
	}

	static void ThreadEntry(void* pParam)
	{
		BaseThread* thiz = (BaseThread*)pParam;
		if (thiz)
		{
			thiz->ThreadWorker();
		}
		_endthread();
	}
private:
	HANDLE m_hThread_;
	//线程状态线程false将要关闭，true线程正在运行
	bool m_bStatus_ = false;
	std::atomic<::ThreadWorker*> m_worker_;
};

/**
 * 线程池.
 */
class BaseThreadPool
{
public:
	BaseThreadPool(size_t size)
	{
		m_threads_.resize(size);
		for (size_t i = 0; i < size; i++)
		{
			m_threads_[i] = new BaseThread();
		}
	}
    BaseThreadPool(){}
	~BaseThreadPool()
	{
		Stop();
		for (size_t i = 0; i < m_threads_.size(); i++)
		{
			delete m_threads_[i];
            m_threads_[i] = nullptr;
		}
		m_threads_.clear();
	}

	bool Invoke()
	{ 
		bool ret = true;
		for (size_t i = 0; i < m_threads_.size(); i++)
		{
			if (m_threads_[i]->Start() == false)
			{
				ret = false;
				break;
			}
		}
		if (ret == false)
		{
			for (size_t i = 0; i < m_threads_.size(); i++)
			{
				m_threads_[i]->Stop();
			}
		}
		return ret;
	}

	void Stop()
	{
		for (size_t i = 0; i < m_threads_.size(); i++)
		{
			m_threads_[i]->Stop();
		}
	}

	//返回-1：线程已满 分配失败
	//返回线程索引：该索引被设为线程工作
	int DispatchWorker(const ThreadWorker& worker)
	{
		int index = -1;
		mux_.lock();
		for (size_t i = 0; i < m_threads_.size(); i++)
		{
			//存在空闲线程
			if (m_threads_[i]->IsIdel())
			{
				m_threads_[i]->UpdateWorker(worker);
				index = i;
				break;
			}
		}
		mux_.unlock();
		return index;
	}

	bool CheckThreadVaild(size_t index)
	{
		if (index < m_threads_.size())
		{
            return m_threads_[index]->IsValid();
		}
		return false;
	}
private:
	std::mutex mux_;
	std::vector<BaseThread*> m_threads_;
};





