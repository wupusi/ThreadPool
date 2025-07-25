

#include <memory>
#include <functional>
#include <vector>
#include <thread>
#include <atomic>
using namespace std;
#include "SyncQueue_4.hpp"

#ifndef WORK_STEALING_POOL_HPP
#define WORK_STEALING_POOL_HPP

    class WorkStealingPool
    {
    public:
        using Task = std::function<void(void)>;

    private:
        size_t m_numThreads;
        SyncQueue<Task> m_queue; // m_queue=> std::vector< std::deque< Task>>
        std::vector<std::shared_ptr<std::thread>> m_threadgroup;
        std::atomic<bool> m_running; // false;
        std::once_flag m_flag;
        int threadIndex() const
        {
            //static size_t num = 0;
            static std::atomic<size_t> num(0);
            return num++ % m_numThreads; // 17
        }
        void Start(int numthreads)
        {
            m_running = true;
            for (int i = 0; i < numthreads; ++i)
            {
                auto tha = std::make_shared<std::thread>(
                    &WorkStealingPool::RunInThread, this, i);
                m_threadgroup.push_back(std::move(tha));
            }
        }
        void RunInThread(const int index)
        {
            while (m_running)
            {
                std::deque<Task> queTask;
                if (m_queue.Take(queTask, index) == 0)
                {
                    for (auto &task : queTask)
                    {
                        if (task)
                        {
                            task();
                        }
                    }
                }
                else
                {
                    int i = threadIndex();
                    if (i != index && m_queue.Take(queTask, i) == 0)
                    {
                        for (auto &task : queTask)
                        {
                            if (task)
                            {
                                task();
                            }
                        }
                    }
                }
            }
        }
        void StopThreadGroup()
        {
            m_queue.Stop();
            m_running = false;
            for (auto &tha : m_threadgroup)
            {
                if (tha && tha->joinable())
                {
                    tha->join();
                }
            }
            m_threadgroup.clear();
        }

    public:
        WorkStealingPool(const int qusize = 500, const int numthreads = 8)
            : m_numThreads(numthreads),
              m_queue(numthreads, qusize),
              m_running(false)
        {
            Start(numthreads);
        }
        ~WorkStealingPool()
        {
            if(m_running)
            {
                Stop();
            }
        }
        void Stop()
        {
            std::call_once(m_flag,[this](){ StopThreadGroup();});
        }

        void AddTask(Task &&task) // void (...)
        {
            int i = threadIndex();
            if (0 != m_queue.Put(std::forward<Task>(task),i))
            {
                fprintf(stderr, "AddTask run task\n");
                task();
            }
        }
        void AddTask(const Task &task)
        {
            int i = threadIndex();
            if (0 != m_queue.Put(task,i))
            {
                fprintf(stderr, "AddTask run task\n");
                task();
            }
        }
        template <class Func, class... Args>
        auto Submit(Func &&func, Args &&...args)
        {
            // typedef decltype(func(args...)) RetType;
            int i = threadIndex();
            using RetType = decltype(func(args...));
            // std::packaged_task<RetType()> task(std::bind(func,args));
            auto task = std::make_shared<std::packaged_task<RetType()>>(
                std::bind(forward<Func>(func), forward<Args>(args)...));

            std::future<RetType> result = task->get_future();

            if (0 != m_queue.Put([task]() -> void
                                 { (*task)(); },i))
            {
                fprintf(stderr, "AddTask run task\n");
                (*task)();
            }
            return result;
        }
    };



#endif