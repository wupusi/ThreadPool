
#include <functional>
#include <list>
#include <memory>
#include <thread>
#include <atomic>
#include <future>
#include <unordered_map>
using namespace std;

#include "SyncQueue_3.hpp"
#ifndef CACHED_THREAD_POOL_HPP
#define CACHED_THREAD_POOL_HPP


    class CachedThreadPool
    {
    public:
        using Task = std::function<void()>;

    private:
        static int KeepAliveTime;
        // std::list<std::shared_ptr<std::thread>> m_threadgroup;
        std::unordered_map<std::thread::id, std::shared_ptr<std::thread>>
            m_threadgroup;
        int m_coreThreadSize;
        int m_maxThreadSize;
        std::atomic_int m_idleThreadSize;
        std::atomic_int m_curThreadSize;
        mutable std::mutex m_mutex;
        std::condition_variable m_threadExit;

        SyncQueue<Task> m_queue;
        std::atomic<bool> m_running; // true; std::atomic_bool;
        std::once_flag m_flag;
        void Start(int numthreads)
        {
            m_running = true;
            m_curThreadSize = numthreads;
            for (int i = 0; i < numthreads; ++i)
            {
                auto tha = std::make_shared<std::thread>(
                    &CachedThreadPool::RunInThread, this);
                std::thread::id tid = tha->get_id();
                // m_threadgroup[tid] = tha;
                m_threadgroup.emplace(tid, tha);
                m_idleThreadSize++;
            }
        }
        void RunInThread()
        {
            auto tid = std::this_thread::get_id();
            auto startTime = std::chrono::high_resolution_clock().now();
            while (m_running)
            {
                Task task;
                if (m_queue.Size() == 0)
                {
                    auto now = std::chrono::high_resolution_clock().now();
                    auto intervalTime = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);
                    std::lock_guard<std::mutex> lock(m_mutex);
                    if (intervalTime.count() >= KeepAliveTime && m_curThreadSize > m_coreThreadSize)
                    {
                        m_threadgroup.find(tid)->second->detach(); // join();

                        m_threadgroup.erase(tid);
                        return;
                    }
                }
                m_queue.Take(task);
                if (task && m_running)
                {
                    m_idleThreadSize--;
                    task();
                    m_idleThreadSize++;
                    startTime = std::chrono::high_resolution_clock().now();
                }
            }
        }
        void StopThreadGroup()
        {
            // m_queue.Stop();
            cout << "m_queue.size(): " << m_queue.Size() << endl;
            m_queue.WaitQueueEmptyStop(); //
            cout << "m_queue.size(): " << m_queue.Size() << endl;
            KeepAliveTime = 1;
            m_coreThreadSize = 0;
            std::unique_lock<std::mutex> lock(m_mutex);
            while (!m_threadgroup.empty())
            {
                m_threadExit.wait_for(lock, std::chrono::milliseconds(100));
            }
            m_running = false;
        }
        void newThread()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            // cout<<"m_idleThreadSize: "<<m_idleThreadSize<<endl;
            // cout<<"m_curThreadSize:  "<<m_curThreadSize<<endl;
            // cout<<"m_maxThreadSize:  "<<m_maxThreadSize<<endl;
            // cout<<(m_idleThreadSize <= 0 && m_curThreadSize < m_maxThreadSize)<<endl;
            if (m_idleThreadSize <= 0 && m_curThreadSize < m_maxThreadSize)
            {
                auto tha = std::make_shared<std::thread>(
                    &CachedThreadPool::RunInThread, this);
                std::thread::id tid = tha->get_id();
                // m_threadgroup[tid] = tha;
                m_threadgroup.emplace(tid, tha);
                m_idleThreadSize++;
                m_curThreadSize++;
            }
        }

    public:
        CachedThreadPool(int initnumthreads = 16, int taskpoolsize = MaxTaskCount)
            : m_coreThreadSize(initnumthreads),
              m_maxThreadSize(std::thread::hardware_concurrency() + 2),
              m_idleThreadSize(0),
              m_curThreadSize(0),
              m_queue(taskpoolsize),
              m_running(false)
        {
            cerr << "m_maxThreadSize: " << m_maxThreadSize << endl;
            Start(m_coreThreadSize);
        }
        ~CachedThreadPool()
        {
            Stop();
        }
        void Stop()
        {
            std::call_once(m_flag, [this]() -> void
                           { StopThreadGroup(); });
        }
        template <typename Func, typename... Args>
        auto AddTask(Func &&func, Args &&...args)
            -> std::future<typename std::invoke_result_t<Func, Args...>>
        {
            using ReturnType = typename std::invoke_result_t<Func, Args...>;

            auto taskPtr = std::make_shared<std::packaged_task<ReturnType()>>(
                std::bind(std::forward<Func>(func), std::forward<Args>(args)...));

            std::function<void()> wrapper = [taskPtr]()
            { (*taskPtr)(); };

            if (0 != m_queue.Put(wrapper))
            {
                fprintf(stderr, "AddTask run task\n");
                wrapper(); // 如果入队失败，直接执行
            }

            newThread();

            return taskPtr->get_future();
        }
        template <class F, class... Args>
        void execute(F &&func, Args &&...args)
        {
            auto task = std::make_shared<std::function<void()>>(std::bind(func, std::forward<Args>(args)...));
            if (m_queue.Put([task]() -> void
                            { (*task)(); }) != 0)
            {
                (*task)();
            }
        }
        template <class Func, class... Args>
        auto Submit(Func &&func, Args &&...args)
        {
            // typedef decltype(func(args...)) RetType;
            using RetType = decltype(func(args...));
            // std::packaged_task<RetType()> task(std::bind(func,args));
            auto task = std::make_shared<std::packaged_task<RetType()>>(
                std::bind(forward<Func>(func), forward<Args>(args)...));

            std::future<RetType> result = task->get_future();

            if (0 != m_queue.Put([task]() -> void
                                 { (*task)(); }))
            {
                fprintf(stderr, "AddTask run task\n");
                (*task)();
            }
            newThread();
            return result;
        }

        int getThreadNum() const { return m_threadgroup.size(); }
    };

    int CachedThreadPool::KeepAliveTime = 60;


#endif