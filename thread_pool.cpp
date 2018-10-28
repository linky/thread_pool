#include <iostream>
#include <vector>
#include <queue>
#include <cstdint>
#include <pthread.h>
#include <unistd.h>

using namespace std;


class Worker
{
public:
    virtual void Do() = 0;
};


class TestWorker : public Worker
{
public:
    explicit TestWorker(int id): m_id(id) {}

    virtual void Do()
    {
        cout << "worker " << m_id << " start" << endl;
        sleep(1);
    }
private:
    int m_id;
};


class ThreadPool
{
public:
    enum Priority {
        PRIORITY_HIGH,
        PRIORITY_NORMAL,
        PRIORITY_LOW,
        PRIORITY_NR
    };

    explicit ThreadPool(size_t threads): m_threads_nr(threads), m_high_cnt(0), m_stop(false), m_workers(threads)
    {
        pthread_cond_init(&m_condition, 0);
        pthread_mutex_init(&m_queue_mutex, 0);

        for (size_t i = 0; i < m_workers.size(); ++i)
            pthread_create(&m_workers[i], 0, Do, (void*)this);
    }

    bool Enqueue(Worker& worker, Priority priorty)
    {
        pthread_mutex_lock(&m_queue_mutex);
        if (m_stop) {
            pthread_mutex_unlock(&m_queue_mutex);
            return false;
        }
        m_queues[priorty].push(&worker);
        pthread_mutex_unlock(&m_queue_mutex);

        pthread_cond_signal(&m_condition);

        return true;
    }

    void Stop()
    {
        pthread_mutex_lock(&m_queue_mutex);
        m_stop = true;
        pthread_mutex_unlock(&m_queue_mutex);

        pthread_cond_broadcast(&m_condition);
        for (size_t i = 0; i < m_workers.size(); ++i) {
            //cerr << "stop " << i << endl;
            pthread_join(m_workers[i], NULL);
        }
    }

    static void* Do(void* arg)
    {
        ThreadPool* thiz = (ThreadPool*)arg;
        Worker* worker = 0;

        pthread_mutex_lock(&thiz->m_queue_mutex);
        for (;;) {
            while (!thiz->m_stop && thiz->IsQueueEmpty())
                pthread_cond_wait(&thiz->m_condition, &thiz->m_queue_mutex);

            if (thiz->m_stop && thiz->IsQueueEmpty())
                break;

            for (size_t i = 0; i < PRIORITY_NR; ++i) {
                if (!thiz->m_queues[i].empty()) {
                    worker = thiz->m_queues[i].front();
                    thiz->m_queues[i].pop();

                    if (i == PRIORITY_HIGH && ++thiz->m_high_cnt >= 3 && !thiz->m_queues[PRIORITY_NORMAL].empty()) {
                        thiz->m_high_cnt = 0;
                        continue; // skip HIGH prior and schedule NORMAL
                    }

                    pthread_cond_broadcast(&thiz->m_condition);
                    pthread_mutex_unlock(&thiz->m_queue_mutex);
                    break;
                }
            }

            worker->Do();
        }
        pthread_mutex_unlock(&thiz->m_queue_mutex);

        return 0;
    }

    virtual ~ThreadPool()
    {
        if (!m_stop)
            Stop();
    }
protected:
    bool IsQueueEmpty() const
    {
        for (size_t i = 0; i < PRIORITY_NR; ++i)
            if (!m_queues[i].empty())
                return false;

        return true;
    }
private:
    ThreadPool();
    ThreadPool(const ThreadPool&);
    ThreadPool& operator=(const ThreadPool&);
private:
    size_t m_threads_nr;
    int m_high_cnt;
    bool m_stop;

    std::vector<pthread_t> m_workers;
    queue<Worker*> m_queues[PRIORITY_NR];

    pthread_mutex_t m_queue_mutex;
    pthread_cond_t m_condition;
};


int main()
{
    ThreadPool pool(3);
    TestWorker testWorker1(1);
    TestWorker testWorker2(2);
    TestWorker testWorker3(3);
    TestWorker testWorker4(4);
    TestWorker testWorker5(5);

    pool.Enqueue(testWorker3, ThreadPool::PRIORITY_HIGH);
    pool.Enqueue(testWorker2, ThreadPool::PRIORITY_NORMAL);
    pool.Enqueue(testWorker1, ThreadPool::PRIORITY_LOW);
    pool.Enqueue(testWorker3, ThreadPool::PRIORITY_HIGH);
    pool.Enqueue(testWorker2, ThreadPool::PRIORITY_NORMAL);
    pool.Enqueue(testWorker4, ThreadPool::PRIORITY_HIGH);
    pool.Enqueue(testWorker5, ThreadPool::PRIORITY_HIGH);
    pool.Enqueue(testWorker1, ThreadPool::PRIORITY_LOW);
    pool.Enqueue(testWorker2, ThreadPool::PRIORITY_NORMAL);
    pool.Enqueue(testWorker4, ThreadPool::PRIORITY_HIGH);
    pool.Enqueue(testWorker1, ThreadPool::PRIORITY_LOW);
    pool.Enqueue(testWorker3, ThreadPool::PRIORITY_HIGH);
    pool.Enqueue(testWorker2, ThreadPool::PRIORITY_NORMAL);
    pool.Enqueue(testWorker1, ThreadPool::PRIORITY_LOW);
    pool.Enqueue(testWorker5, ThreadPool::PRIORITY_HIGH);
    pool.Enqueue(testWorker3, ThreadPool::PRIORITY_HIGH);
    pool.Enqueue(testWorker2, ThreadPool::PRIORITY_NORMAL);
    pool.Enqueue(testWorker1, ThreadPool::PRIORITY_LOW);
    pool.Enqueue(testWorker3, ThreadPool::PRIORITY_HIGH);
    pool.Enqueue(testWorker2, ThreadPool::PRIORITY_NORMAL);
    pool.Enqueue(testWorker4, ThreadPool::PRIORITY_HIGH);
    pool.Enqueue(testWorker5, ThreadPool::PRIORITY_HIGH);
    pool.Enqueue(testWorker1, ThreadPool::PRIORITY_LOW);
    pool.Enqueue(testWorker2, ThreadPool::PRIORITY_NORMAL);
    pool.Enqueue(testWorker4, ThreadPool::PRIORITY_HIGH);

    pool.Stop();
}