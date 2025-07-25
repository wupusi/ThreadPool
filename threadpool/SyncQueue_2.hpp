
#include <vector>
#include <list>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <iostream>
using namespace std;
#if 1
#ifndef SYNC_QUEUE_2_HPP
#define SYNC_QUEUE_2_HPP


    const int MaxTaskCount = 500;
    template <class T> // T= Task
    class SyncQueue
    {
    private:
        // std::list<T> m_queue;
        std::deque<T> m_queue;
        mutable std::mutex m_mutex;
        std::condition_variable m_notEmpty; // xiaofeizhe
        std::condition_variable m_notFull;  //
        int m_maxSize;                      //
        bool m_needStop;                    // false run ; true stop;

        bool IsEmpty() const
        {
            // std::lock_guard<std::mutex> lock(m_mutex);
            bool empty = m_queue.empty();
            if (empty)
            {
                cerr << "m_queue empty wait... " << endl;
            }
            return empty;
        }
        bool IsFull() const
        {
            // std::lock_guard<std::mutex> lock(m_mutex);
            bool full = m_queue.size() >= m_maxSize;
            if (full)
            {
                cerr << "m_queue full wait ..." << endl; //
            }
            return full;
        }
        template <class F>
        int Add(F &&task) // 5000 ms;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            int ret = -1;
            bool tag = m_notFull.wait_for(lock,
                                          std::chrono::milliseconds(1),
                                          [this]() -> bool
                                          {
                                              return !IsFull() || m_needStop;
                                          });
            if (tag && !m_needStop)
            {
                m_queue.push_back(std::forward<F>(task));
                ret = 0;
            }
            m_notEmpty.notify_all();
            return ret;
        }

    public:
        SyncQueue(int maxsize = MaxTaskCount)
            : m_maxSize(maxsize), m_needStop(false)
        {
        }
        ~SyncQueue() { Stop(); }
        SyncQueue(const SyncQueue &) = delete;
        SyncQueue &operator=(const SyncQueue &) = delete;

        int Put(const T &task)
        {
            return Add(task);
        }
        int Put(T &&task)
        {
            return Add(std::forward<T>(task));
        }

        void Take(T &task)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            while (IsEmpty() && !m_needStop)
            {
                m_notEmpty.wait(lock);
            }
            if (!m_needStop)
            {
                task = m_queue.front();
                m_queue.pop_front();
            }
            m_notFull.notify_all();
        }
        void Take(std::deque<T> &queue)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            while (IsEmpty() && !m_needStop)
            {
                m_notEmpty.wait(lock);
            }
            if (!m_needStop)
            {
                queue = std::move(m_queue);
            }
            m_notFull.notify_all();
        }
        void Stop()
        {

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_needStop = true;
            }
            m_notFull.notify_all();
            m_notEmpty.notify_all();
        }
        bool Empty() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_queue.empty();
        }
        bool Full() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_queue.size() >= m_maxSize;
        }
        size_t Size() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_queue.size();
        }
        size_t Count() const
        {
            return m_queue.size();
        }
    };


#endif
#endif 