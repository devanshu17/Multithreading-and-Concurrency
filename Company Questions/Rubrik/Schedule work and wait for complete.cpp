#include <bits/stdc++.h>
using namespace std;

template <typename T>
class Scheduler
{
    queue<T> q;
    mutex m;
    condition_variable m_cond;

    void Runner()
    {
        while (1)
        {
            unique_lock<mutex> l_lock(m);
            m_cond.wait(l_lock, [this]() -> bool { return !q.empty(); });

            auto work = q.front(); q.pop();
            l_lock.unlock();

            try
            {
                work();
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
            }
            catch(...)
            {
                cout<<" caught unknown exception"<<endl;
            }

            m_cond.notify_all();
        }
    }

    public:

    Scheduler()
    {
        std::thread t(Runner, this);
        t.detach();
    }

    void Schedule(T&& p_work)
    {
        unique_lock<mutex> l_lock(m);
        q.push(forward<T>(p_work));
        m_cond.notify_all();
    }

    void WaitForWork()
    {
        unique_lock<mutex> l_lock(m);
        m_cond.wait(l_lock, [this]() -> bool { return q.empty(); });
    }
};

int main()
{
    Scheduler<function<void()>> scheduler;

    // Schedule some work
    while (1)
    {
        scheduler.Schedule([]() -> void {
            cout << "Hello from the first task!" << endl;
            this_thread::sleep_for(chrono::seconds(3));

        });

        scheduler.Schedule([]() -> void {
            cout << "Hello from the second task!" << endl;
            this_thread::sleep_for(chrono::seconds(3));
        });

        if (rand() % 500 == 0)
        {
            break;
        }
    }

    cout<<"here waiting "<<endl;

    scheduler.WaitForWork();
    // Wait for a while to let the tasks finish

    return 0;
}
