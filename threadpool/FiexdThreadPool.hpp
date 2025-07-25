
#include <functional>
#include <list>
#include <memory>
#include <thread>
#include <atomic>
#include <future>
using namespace std;

#include "SyncQueue_2.hpp"
#ifndef FIEXD_THREAD_POOL_HPP
#define FIEXD_THREAD_POOL_HPP


    class FiexdThreadPool
    {
    public:
        using Task = std::function<void(void)>;

    private:
        std::list<std::shared_ptr<std::thread>> m_threadgroup;
        SyncQueue<Task> m_queue;
        std::atomic<bool> m_running; // true; std::atomic_bool;
        std::once_flag m_flag;
        void Start(int numthreads)
        {
            m_running = true;
            for (int i = 0; i < numthreads; ++i)
            {
                m_threadgroup.push_back(std::make_shared<std::thread>(
                    &FiexdThreadPool::RunInThread, this));
            }
            // new std::thread(&FiexdThreadPool::RunInThread,this);
        }
        void RunInThread()
        {
            while (m_running)
            {
                Task task;
                m_queue.Take(task);
                if (m_running && task)
                {
                    task();
                }
            }
        }
        void StopThreadGroup()
        {
            m_queue.Stop();
            m_running = false;
            for (auto &thr : m_threadgroup)
            {
                thr->join();
            }
            m_threadgroup.clear();
        }

    public:
        FiexdThreadPool(int numthreads = std::thread::hardware_concurrency())
            : m_queue(MaxTaskCount), m_running(false)
        {
            Start(numthreads);
        }
        ~FiexdThreadPool()
        {
            Stop();
        }
        void Stop()
        {
            std::call_once(m_flag, [this]() -> void
                           { StopThreadGroup(); });
        }
        void AddTask(Task &&task)  // void (...)
        {
            if (0 != m_queue.Put(std::forward<Task>(task)))
            {
                fprintf(stderr,"AddTask run task\n");
                task();
            }
        }
        void AddTask(const Task &task)
        {
            if (0 != m_queue.Put(task))
            {
                fprintf(stderr,"AddTask run task\n");
                task();
            }
        }
        template<class Func,class... Args>
        auto Submit(Func && func,Args&&...args)
        {
            //typedef decltype(func(args...)) RetType;
            using RetType = decltype(func(args...));
           // std::packaged_task<RetType()> task(std::bind(func,args));
            auto task = std::make_shared< std::packaged_task<RetType()> >(
                std::bind(forward<Func>(func),forward<Args>(args)...) 
            );
           
            std::future<RetType> result = task->get_future();
           
            if (0 != m_queue.Put([task]()->void{ (*task)();}))
            {
                fprintf(stderr,"AddTask run task\n");
                (*task)();
            }
            return result;
            
        }
    };

#endif