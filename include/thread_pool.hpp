#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <mutex>			  //	std::mutex
#include <condition_variable> //	std::condition_variable
#include <memory>			  //	std::shared_pointer
#include <unordered_map>	  // 	std::unordered_map
#include <utility>			  //    std:: pair
#include <atomic>			  //    std:atomic<boo>

#include "worker_thread.hpp"
#include "waitable_queue.hpp" // levi::WaitableQueue
#include "priority_queue.hpp"



namespace levi
{

	class ThreadPool
	{
	public:
		enum Priority
		{
			LOW,
			NORMAL,
			HIGH
		};

		explicit ThreadPool(std::size_t threadsNum_);
		~ThreadPool() noexcept;
		ThreadPool(const ThreadPool &other_) = delete;
		ThreadPool(const ThreadPool &&other_) = delete;
		ThreadPool &operator=(const ThreadPool &&other_) = delete;
		ThreadPool &operator=(const ThreadPool &other_) = delete;

		class ITask;
		

		void Pause();
		void Resume();
		void SetNumOfThreads(std::size_t newThreadsNum_);
		void AddTask(std::shared_ptr<ITask> p_task_, Priority priority_ = NORMAL);

	private:
		class KillThreadTask;
		class StopThreadTask;

		typedef std::shared_ptr<ITask> ITaskPtr;
		typedef std::pair<ITaskPtr, int> TaskPriorityPair;
		std::atomic_bool m_is_pause;


		const int KILL_PRIORITY = 5;
		const int STOP_PRIORITY = 6;
		const int PAUSE_PRIORITY = 7;

		class CompareFunctor
		{
		public:
			bool operator()(const TaskPriorityPair &p1, TaskPriorityPair &p2) const;
		};

		std::atomic_size_t m_working_thread_size;

		WaitableQueue<TaskPriorityPair, PQWrapper<TaskPriorityPair, std::vector<TaskPriorityPair>, CompareFunctor>> m_tasksQueue;
		std::unordered_map<std::thread::id, std::shared_ptr<WorkerThread>> m_map;
		WaitableQueue<std::thread::id> m_wq_threads_id;

		std::mutex m_mutex;
		std::condition_variable m_cv;

		void StopThreads(size_t num_of_threads);
		void ThreadExec();
	}; // ThreadPool
	
	class ThreadPool::ITask
	{
	public:
		ITask() = default;
		virtual ~ITask() = default;

    	ITask(const ITask&) = delete;
		ITask(const ITask &&other_) = delete;
    	ITask& operator=(const ITask&) = delete;
    	ITask& operator=(ITask&&) = delete;
		
		virtual void Execute() = 0; 
	};


	class ThreadPool::StopThreadTask : public ThreadPool::ITask
	{
	public:
		StopThreadTask(ThreadPool *pool);
		~StopThreadTask() = default;
		void Execute();

		StopThreadTask(const StopThreadTask &other_) = delete;
		StopThreadTask(const StopThreadTask &&other_) = delete;
		StopThreadTask &operator=(const StopThreadTask &&other_) = delete;
		StopThreadTask &operator=(const StopThreadTask &other_) = delete;
	private:
		ThreadPool *m_pool;
	};


	class ThreadPool::KillThreadTask : public ThreadPool::ITask
	{
	public:
		KillThreadTask(ThreadPool *pool);
		~KillThreadTask() = default;
		void Execute();

		KillThreadTask(const KillThreadTask &other_) = delete;
		KillThreadTask(const KillThreadTask &&other_) = delete;
		KillThreadTask &operator=(const KillThreadTask &&other_) = delete;
		KillThreadTask &operator=(const KillThreadTask &other_) = delete;
	
	private:
		ThreadPool *m_pool;
	};

	template <typename ReturnType, typename... Args>
	class FutureTask : public ThreadPool::ITask
	{
	public:
		FutureTask(ReturnType func_(Args...), Args... args_) : m_func(std::bind(func_, args_...)), m_res_is_ready(false) {}

		~FutureTask() = default;

		void Execute() override
		{
			m_result = m_func();
			m_res_is_ready = true;
			m_cvar.notify_one();
		}

		ReturnType GetResult() const
		{
			std::unique_lock<std::mutex> lock(m_mtx);
			m_cvar.wait(lock, [this]() { return true == m_res_is_ready; });
			return m_result;
		}

		FutureTask(const FutureTask &other_) = delete;
		FutureTask(const FutureTask &&other_) = delete;
		FutureTask &operator=(const FutureTask &&other_) = delete;
		FutureTask &operator=(const FutureTask &other_) = delete;

	private:
		std::function<ReturnType(void)> m_func;
		ReturnType m_result;
		bool m_res_is_ready;
		mutable std::condition_variable m_cvar;
		mutable std::mutex m_mtx;
	};
} // levi

#endif /* thread_pool.hpp */
