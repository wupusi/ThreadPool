
#include <vector>
#include <list>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <iostream>
using namespace std;
#ifndef SYNC_QUEUE_3_HPP
#define SYNC_QUEUE_3_HPP


    const int MaxTaskCount = 1000;
    template <class T> // T= Task
    class SyncQueue
    {
    private:
        // std::list<T> m_queue;
        std::deque<T> m_queue;
        mutable std::mutex m_mutex;
        std::condition_variable m_notEmpty; // xiaofeizhe
        std::condition_variable m_notFull;  //
        std::condition_variable m_waitStop;
        size_t m_waitTime;                  // 1s
        int m_maxSize;                      //
        bool m_needStop;                    // false run ; true stop;

        bool IsEmpty() const
        {
            // std::lock_guard<std::mutex> lock(m_mutex);
            bool empty = m_queue.empty();
            if (empty)
            {
                //cerr << "m_queue empty wait... " << endl;
            }
            return empty;
        }
        bool IsFull() const
        {
            // std::lock_guard<std::mutex> lock(m_mutex);
            bool full = m_queue.size() >= m_maxSize;
            if (full)
            {
                //cerr << "m_queue full wait ..." << endl; //
               // cerr << "m_maxSize: " << m_maxSize << endl;
                //cerr << "m_queue.size(): " << m_queue.size() << endl;
            }
            return full;
        }
        template <class F>
        int Add(F &&task) // 5000 ms;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            int ret = -1;
            bool tag = m_notFull.wait_for(lock,
                                          std::chrono::milliseconds(m_waitTime),
                                          [this]() -> bool
                                          {
                                              return (!IsFull() || m_needStop);
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
                m_queue.push_back(std::forward<F>(task));
                ret = 0;
            }
            m_notEmpty.notify_all();
            return ret;
        }

    public:
        SyncQueue(int maxsize = MaxTaskCount, const size_t timeout = 10) // ms
            : m_maxSize(maxsize),
              m_needStop(false),
              m_waitTime(timeout)
        {
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

        int Put(const T &task)
        {
            return Add(task);
        }
        int Put(T &&task)
        {
            return Add(std::forward<T>(task));
        }

        int Take(T &task)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            int ret = -1;
            bool tag = m_notEmpty.wait_for(lock,
                                           std::chrono::milliseconds(m_waitTime),
                                           [this]() -> bool
                                           {
                                               return (m_needStop || !IsEmpty());
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
                
                task = m_queue.front();
                m_queue.pop_front();
                ret = 0;
            }
            m_notFull.notify_all(); 
            return ret;
        }
        int Take(std::deque<T> &queue)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            int ret = -1;
            bool tag = m_notEmpty.wait_for(lock,
                                           std::chrono::milliseconds(m_waitTime),
                                           [this]() -> bool
                                           {
                                               return (m_needStop || !IsEmpty());
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
                queue = std::move(m_queue);
                ret = 0;
            }
            m_notFull.notify_all();
            return ret;
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
        void WaitQueueEmptyStop()
        {
            cout<<"WaitQueueEmptyStop() "<<endl;
            std::unique_lock<std::mutex> locker(m_mutex);
            cout<<" locker(m_mutex)"<<endl;
            while(!IsEmpty())
            {
                cout<<"SysncQueue:: wait_for: IsEmpty():" <<!IsEmpty();
                m_waitStop.wait_for(locker,std::chrono::milliseconds(m_waitTime));
            }
            cout<<" SyncQueue:: m_queue:size() "<<m_queue.size()<<endl;
            m_needStop = true;
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