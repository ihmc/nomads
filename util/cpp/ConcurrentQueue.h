#ifndef INCL_CONCURRENT_QUEUE
#define INCL_CONCURRENT_QUEUE

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace NOMADSUtil
{
    template <typename T>
    class ConcurrentQueue
    {
        public:
            ConcurrentQueue (void);
            ~ConcurrentQueue (void);

            T pop (void);
            void pop (T& item);
            void push (const T& item);

        private:
            std::queue<T> _queue;
            std::mutex _m;
            std::condition_variable _cv;
    };

    template <typename T>
    ConcurrentQueue<T>::ConcurrentQueue (void) {}

    template <typename T>
    ConcurrentQueue<T>::~ConcurrentQueue (void) {}

    template <typename T>
    T ConcurrentQueue<T>::pop (void)
    {
        std::unique_lock<std::mutex> mlock (_m);
        while (_queue.empty ()) {
            _cv.wait (mlock);
        }
        auto val = _queue.front();
        _queue.pop();
        return val;
    }

    template <typename T>
    void ConcurrentQueue<T>::pop (T& item)
    {
        std::unique_lock<std::mutex> mlock (_m);
        while (_queue.empty()) {
            _cv.wait (mlock);
        }
        item = _queue.front();
        _queue.pop();
    }

    template <typename T>
    void ConcurrentQueue<T>::push (const T& item)
    {
        std::unique_lock<std::mutex> mlock (_m);
        _queue.push (item);
        mlock.unlock();
        _cv.notify_one();
    }
}

#endif  /* INCL_CONCURRENT_QUEUE */

