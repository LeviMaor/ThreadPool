#ifndef WAITABLE_QUEUE
#define WAITABLE_QUEUE

#include <condition_variable>     // std::condition_variable
#include <mutex>                  // std::timed_mutex
#include <queue>                  // std::queue

namespace levi
{

template<class T, class CONTAINER = std::queue<T>>
class WaitableQueue
{
public:
	WaitableQueue() = default;
	~WaitableQueue() = default;

	WaitableQueue(const WaitableQueue& other_) = delete;
	WaitableQueue operator=(const WaitableQueue& other_) = delete;
	WaitableQueue(const WaitableQueue&& other_) = delete;
	WaitableQueue operator=(const WaitableQueue&& other_) = delete;


	void Push(const T& data_);
	void Pop(T& out_);
	bool Pop(T& out_, const std::chrono::milliseconds& timeout_);
	bool IsEmpty() const;

private:
	CONTAINER m_queue;
	mutable std::timed_mutex m_mutex;
	std::condition_variable_any m_cv;
};

template<class T, class CONTAINER>
void WaitableQueue<T, CONTAINER>::Push(const T& data_)
{
    {
    	std::unique_lock<std::timed_mutex> lock(m_mutex);
    
		m_queue.push(data_);
    }
    
    m_cv.notify_one();
}

template<class T, class CONTAINER>
void WaitableQueue<T, CONTAINER>::Pop(T& out) 
{
	std::unique_lock<std::timed_mutex> lock(m_mutex);
	m_cv.wait(lock, [this]() { return false == m_queue.empty();});
	out = m_queue.front();
	m_queue.pop();
}


template<class T, class CONTAINER>
bool WaitableQueue<T, CONTAINER>::Pop(T& out_, const std::chrono::milliseconds& timeout_)
{

	using namespace std::chrono;

    std::unique_lock<std::timed_mutex> lock(m_mutex, std::defer_lock);

	milliseconds startTime = duration_cast<milliseconds>
	(system_clock::now().time_since_epoch());

	if (false == lock.try_lock_for(timeout_))
	{
		std::cout << "mutex is not lock" << std::endl;
		return false;
	}
    
	milliseconds endTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    milliseconds TimeInterval = timeout_ - (endTime - startTime);
    
	if (false == m_cv.wait_for(lock, TimeInterval, [this] { return false == m_queue.empty(); })) 
	{
		return false; 
	}
    
	out_ = m_queue.front();
	m_queue.pop();

	return true; 
}


template<class T, class CONTAINER>
bool WaitableQueue<T, CONTAINER>::IsEmpty() const
{
    std::unique_lock<std::timed_mutex> lock(m_mutex);
    
    return m_queue.empty();
}


} // levi


#endif // WAITABLE_QUEUE.HPP


