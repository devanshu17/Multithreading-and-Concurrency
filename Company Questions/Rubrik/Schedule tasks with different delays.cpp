#include <bits/stdc++.h>
#include <thread>
#include <chrono>
#include <condition_variable>
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
            cout<<" worker started "<<endl;
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
        busyWorkers = 0;
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

void ScheduleTask(function<void()> p_task, ThreadPool& p_consumers)
{
    p_consumers.EnqueueTask(p_task);
}

void Schedule(int delay, ThreadPool& p_consumers)
{
    auto l_task = []() -> void {
        cout<<" running single time task"<<endl;
        this_thread::sleep_for(chrono::milliseconds(rand() % 400));
    };

    this_thread::sleep_for(chrono::milliseconds(delay));

    ScheduleTask(l_task, p_consumers);
}

void ScheduleRegular(int delay, int interval, ThreadPool& p_consumers, atomic<bool>& stop)
{
    auto l_task = []() -> void {
        cout<<" running regular time task"<<endl;
        this_thread::sleep_for(chrono::milliseconds(rand() % 400));
    };

    this_thread::sleep_for(chrono::milliseconds(delay));

    while (!stop.load())
    {
        ScheduleTask(l_task, p_consumers);
        this_thread::sleep_for(chrono::milliseconds(interval));
    }
}

void ScheduleRegularSingle(int delay, int interval, ThreadPool& p_consumers, atomic<bool>& stop)
{
    mutex l_mutex;
    condition_variable l_cond;
    bool finished = false;

    auto l_task = [&]() -> void {
        cout<<" running regular single task"<<endl;
        this_thread::sleep_for(chrono::milliseconds(rand() % 400));
        unique_lock<mutex> l_lock(l_mutex);
        finished = true;
        l_cond.notify_one();
    };

    this_thread::sleep_for(chrono::milliseconds(delay));

    while (!stop.load())
    {
        unique_lock<mutex> l_lock(l_mutex);
        finished = false;
        l_lock.unlock();
        ScheduleTask(l_task, p_consumers);
        l_lock.lock();
        l_cond.wait(l_lock, [&]() -> bool { return finished; });
        this_thread::sleep_for(chrono::milliseconds(interval));
    }
}

int main()
{
    ThreadPool l_consumers(10);
    atomic<bool> stop = false;

    std::thread t1([&]() -> void {
        ScheduleRegular(1, 1, l_consumers, stop);
    });

    std::thread t2([&]() -> void {
        ScheduleRegularSingle(1, 1, l_consumers, stop);
    });

    cout<<" here "<<endl;

    while (1)
    {
        this_thread::sleep_for(chrono::milliseconds(1));
        Schedule(1, l_consumers);

        if (rand() % 1000 == 0)
        {
            stop.store(true);
            break;
        }
    }

    t1.join();
    t2.join();

    return 0;
}