#ifndef WORKERTHREAD_HPP
#define WORKERTHREAD_HPP

#include <thread>			  //	std::thread
#include <future>			  //	std::function

namespace levi
{
    class WorkerThread
    {
    public:
        WorkerThread(std::function<void()> threadFunc) : m_thread(threadFunc), m_thread_id(m_thread.get_id())
        {
            //empty
        }

        ~WorkerThread()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if(m_thread.joinable())
            {
                m_thread.join();
            }
        }

        std::thread::id GetID() const
        {
            return m_thread_id;
        }

        void JoinThread()
        {
            if(m_thread.joinable())
            {
                m_thread.join();
            }
        }
    private:
        std::mutex m_mutex;
        std::thread m_thread;
        std::thread::id m_thread_id;
    };

} // levi

#endif /* worker_thread.hpp */
