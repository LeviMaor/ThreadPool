#include <iostream>


#include "thread_pool.hpp"

namespace levi
{
	class PauseThreadTask : public ThreadPool::ITask
	{
	public:
        PauseThreadTask(std::mutex& mutex, std::condition_variable& cv, std::atomic_bool& isPause): m_mutex(mutex), m_cv(cv), m_is_pause(isPause)
        {
            //empty
        }
		~PauseThreadTask() = default;
		void Execute()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this]() { return false == m_is_pause;});
        };


		PauseThreadTask(const PauseThreadTask &other_) = delete;
		PauseThreadTask(const PauseThreadTask &&other_) = delete;
		PauseThreadTask &operator=(const PauseThreadTask &&other_) = delete;
		PauseThreadTask &operator=(const PauseThreadTask &other_) = delete;

        private:
            std::mutex &m_mutex;
            std::condition_variable &m_cv;
            std::atomic_bool &m_is_pause;
	};

    ThreadPool::ThreadPool(std::size_t threadsNum_): m_is_pause(false), m_working_thread_size(threadsNum_) 
    {

        std::function<void(void)> thread_func = ( [this]{ ThreadExec(); });
        for (std::size_t i = 0; i < threadsNum_; ++i)
        {
            std::shared_ptr<WorkerThread> i_thread = std::make_shared<WorkerThread>(thread_func);
            m_map[i_thread->GetID()] = i_thread;
        }
        
    }


    ThreadPool::~ThreadPool() noexcept
    {
        StopThreads(0);
        m_map.clear();

    }

    void ThreadPool::Pause()
    {
        m_is_pause = true;
        std::shared_ptr<PauseThreadTask> pause_task =  std::make_shared<PauseThreadTask>(m_mutex, m_cv, m_is_pause);;
        TaskPriorityPair pair = {pause_task, PAUSE_PRIORITY};
        for(std::size_t i = 0; i < m_working_thread_size; ++i)
        {
            m_tasksQueue.Push(pair);
        }

    }

    void ThreadPool::Resume()
    {
        m_is_pause = false;
        m_cv.notify_all();
    }

    void ThreadPool::StopThreads(size_t new_num_of_threads)
    {
        for(std::size_t i = 0; i < m_working_thread_size - new_num_of_threads; ++i)
        {
            ITaskPtr task_ptr_stop = std::make_shared<StopThreadTask>(this);
            TaskPriorityPair pair_stop = {task_ptr_stop, STOP_PRIORITY};  
            m_tasksQueue.Push(pair_stop);
         }
    }


    void ThreadPool::SetNumOfThreads(std::size_t newThreadsNum_)
    {   
        if(m_working_thread_size > newThreadsNum_)
        {
            StopThreads(newThreadsNum_);
            
            for(std::size_t i = 0; i < m_working_thread_size - newThreadsNum_; ++i)
            {
                ITaskPtr task_ptr_kill = std::make_shared<KillThreadTask>(this);
                TaskPriorityPair pair_kill = {task_ptr_kill, KILL_PRIORITY};
                m_tasksQueue.Push(pair_kill);
            }
        }

        else if((m_working_thread_size < newThreadsNum_))
        {
            std::function<void()> thread_func = ([this](){ ThreadExec(); });
            for (std::size_t i = 0; i < newThreadsNum_ - m_working_thread_size; ++i)
            {
                std::shared_ptr<WorkerThread> i_thread = std::make_shared<WorkerThread>(thread_func);
                m_map[i_thread->GetID()] = i_thread;
            }
        
        }
        m_working_thread_size = newThreadsNum_;
    }

    void ThreadPool::AddTask(std::shared_ptr<ITask> p_task_, Priority priority_)
    {
        TaskPriorityPair pair = {p_task_, priority_};
        m_tasksQueue.Push(pair);
    }

    void ThreadPool::ThreadExec()
    {
        
        TaskPriorityPair pair;

        while(1)
        {
            m_tasksQueue.Pop(pair);

            pair.first->Execute();
            if(pair.second == STOP_PRIORITY) 
            {
                break;
            }
            
        }

    }

    ThreadPool::StopThreadTask::StopThreadTask(ThreadPool *pool) : m_pool(pool)
    {
        //Empty
    }

    void ThreadPool::StopThreadTask::Execute()
    {
        m_pool->m_wq_threads_id.Push(std::this_thread::get_id()); 
    }


    ThreadPool::KillThreadTask::KillThreadTask(ThreadPool *pool) : m_pool(pool)
    {
    }

    void ThreadPool::KillThreadTask::Execute()
    {
        std::thread::id threadId;
        m_pool->m_wq_threads_id.Pop(threadId);
        m_pool->m_map.erase(threadId);
    }

    bool ThreadPool::CompareFunctor::operator()(const TaskPriorityPair &p1, TaskPriorityPair &p2) const
    {
        return (p1.second < p2.second); 
    }


} // levi

