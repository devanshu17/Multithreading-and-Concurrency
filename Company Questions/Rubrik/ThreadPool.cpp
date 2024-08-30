#include <bits/stdc++.h>
using namespace std;

class ThreadPool
{
    int size;
    queue<function<void()>> m_tasks;
    mutex m_readWrite;
    vector<thread> m_workers;
    bool stop;
    condition_variable m_cond;
    int busyWorkers;

    void LaunchNewWorker()
    {
        unique_lock<mutex> l_lock(m_readWrite);

        if (size > m_workers.size() && busyWorkers == m_workers.size())
        {
            m_workers.push_back(std::thread([this]() -> void {
                    while (!stop)
                    {
                        unique_lock<mutex> l_lock(m_readWrite);
                        m_cond.wait(l_lock, [this]() -> bool { return !m_tasks.empty() || stop; });

                        if (m_tasks.size() && !stop)
                        {
                            auto currTask = m_tasks.front(); m_tasks.pop();
                            busyWorkers++;
                            l_lock.unlock();
                            try
                            {
                                currTask();
                            }
                            catch(...)
                            {
                                cout<<" task has thrown exception "<<endl;
                            }

                            l_lock.lock();
                            busyWorkers--;
                        }
                    }
                }));
        }
    }

    public:
    ThreadPool(int p_capacity)
    {
        size = p_capacity;
        stop = false;
    }

    void EnqueueTask(function<void()> p_task)
    {
        unique_lock<mutex> l_lock(m_readWrite);
        m_tasks.push(p_task);
        m_cond.notify_one();
        l_lock.unlock();
        LaunchNewWorker();
    }

    ~ThreadPool()
    {
        unique_lock<mutex> l_lock(m_readWrite);
        stop = true;
        l_lock.unlock();

        m_cond.notify_all();

        for (auto& t : m_workers)
        {
            t.join();
        }
    }
};

int main()
{

    return 0;
}