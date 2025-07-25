
#include <vector>
#include <list>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <future>
#include <chrono>
using namespace std;
#ifndef SYNC_QUEUE_4_HPP
#define SYNC_QUEUE_4_HPP


    const int MaxTaskCount = 500;
    template <class T> // T= Task
    class SyncQueue
    {
    private:
        // std::list<T> m_queue;
        //std::deque<T> m_queue;
        // std::vector<std::list<T> > m_queue;
        std::vector<std::deque<T> > m_queue;
        size_t m_bucketSize; // vector.resize(16);
        size_t m_maxSize;    // vector[0].deque.size()
        mutable std::mutex m_mutex;
        std::condition_variable m_notEmpty; // xiaofeizhe
        std::condition_variable m_notFull;  //
        std::condition_variable m_waitStop;
        size_t m_waitTime; // 1s
        bool m_needStop;   // false run ; true stop;

        bool IsEmpty(const int index) const
        {
            // std::lock_guard<std::mutex> lock(m_mutex);
            bool empty = m_queue[index].empty();
            if (empty)
            {
                cerr << "m_queue[" << index << "] empty wait... " << endl;
            }
            return empty;
        }
        bool IsFull(const int index) const
        {
            // std::lock_guard<std::mutex> lock(m_mutex);
            bool full = m_queue[index].size() >= m_maxSize;
            if (full)
            {
                cerr << "m_queue[" << index << " ] full wait ..." << endl; //
                                                                           // cerr << "m_maxSize: " << m_maxSize << endl;
                // cerr << "m_queue.size(): " << m_queue.size() << endl;
            }
            return full;
        }
        template <class F>
        int Add(F &&task, const int index) // 5000 ms;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            int ret = -1;
            bool tag = m_notFull.wait_for(lock,
                                          std::chrono::milliseconds(m_waitTime),
                                          [this, &index]() -> bool
                                          {
                                              return (!IsFull(index) || m_needStop);
                                          });
            if (m_needStop)
            {
                ret = -1; // SyncQueue stop;
            }
            else if (!tag)
            {
                ret = -2; // m_queue full
            }
            else
            {
                m_queue[index].push_back(std::forward<F>(task));
                ret = 0;
            }
            m_notEmpty.notify_all();
            return ret;
        }

    public:
        SyncQueue(int bucketsize = 16 , int maxsize = MaxTaskCount, const size_t timeout = 100) // ms
            :m_bucketSize(bucketsize),  // 16* 500 
            m_maxSize(maxsize),
              m_needStop(false),
              m_waitTime(timeout)
        {
            m_queue.resize(m_bucketSize);
        }
        ~SyncQueue()
        {
            if (!m_needStop)
            {
                Stop();
            }
        }
        SyncQueue(const SyncQueue &) = delete;
        SyncQueue &operator=(const SyncQueue &) = delete;

        int Put(const T &task, const int index)
        {
            return Add(task, index);
        }
        int Put(T &&task, const int index)
        {
            return Add(std::forward<T>(task), index);
        }

        int Take(std::deque<T> &queue, const int index)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            int ret = -1;
            bool tag = m_notEmpty.wait_for(lock,
                                           std::chrono::milliseconds(m_waitTime),
                                           [this, &index]() -> bool
                                           {
                                               return (m_needStop || !IsEmpty(index));
                                           });
            if (m_needStop)
            {
                ret = -1;
            }
            else if (!tag)
            {
                ret = -3; // m_queue empty;
            }
            else
            {
                queue = std::move(m_queue[index]);
                ret = 0;
            }
            m_notFull.notify_all();
            return ret;
        }
        int Take(T &task, const int index)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            int ret = -1;
            bool tag = m_notEmpty.wait_for(lock,
                                           std::chrono::milliseconds(m_waitTime),
                                           [this, &index]() -> bool
                                           {
                                               return (m_needStop || !IsEmpty(index));
                                           });
            if (m_needStop)
            {
                ret = -1;
            }
            else if (!tag)
            {
                ret = -3; // m_queue empty;
            }
            else
            {
                task = m_queue[index].front();
                m_queue[index].pop_front();
                ret = 0;
            }
            m_notFull.notify_all();
            return ret;
        }
        void Stop()
        {
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                for(int i = 0;i<m_bucketSize;++i)
                {
                    while(!m_needStop && !IsEmpty(i))  //??
                    {
                        m_notFull.wait_for(lock,std::chrono::seconds(m_waitTime));
                    }
                }
                m_needStop = true;
            }
            m_notFull.notify_all();
            m_notEmpty.notify_all();
        }
        void WaitQueueEmptyStop()
        {
            #if 0 
            cout << "WaitQueueEmptyStop() " << endl;
            std::unique_lock<std::mutex> locker(m_mutex);
            cout << " locker(m_mutex)" << endl;
            while (!IsEmpty())
            {
                cout << "SysncQueue:: wait_for: IsEmpty():" << !IsEmpty();
                m_waitStop.wait_for(locker, std::chrono::milliseconds(m_waitTime));
            }
            cout << " SyncQueue:: m_queue:size() " << m_queue.size() << endl;
            m_needStop = true;
            m_notFull.notify_all();
            m_notEmpty.notify_all();
            #endif 
        }
        bool Empty(const int index ) const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_queue[index].empty();
        }
        bool Full(const int index) const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_queue[index].size() >= m_maxSize;
        }
        size_t Size(const int index) const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_queue[index].size();
        }
        size_t Count(const int index) const
        {
            return m_queue[index].size();
        }
        size_t TaskTotalSize() const 
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            size_t sum = 0;
            for(auto &xdeq: m_queue)
            {
                sum += xdeq.size();
            }
            return sum;
        }
        void PrintTaskInfo() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            int n = m_queue.size();
            for(int i = 0;i<n;++i)
            {
                printf("%d 桶中的任务数量 %d \n",i,m_queue[i].size());
            }
            printf("\n---------------------------------\n");
        }
    };


#endif