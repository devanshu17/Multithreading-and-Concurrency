#include <thread>
#include <iostream>
#include <mutex>
#include <chrono>
#include <deque>
#include <bits/stdc++.h>

using namespace std;

template<typename T>
class BoundedBlockingQueue
{
    private:
    deque<T> m_dq;
    mutex m_rwMutex;
    condition_variable m_cond;
    size_t capacity;

    public:
    BoundedBlockingQueue(int n)
    {
        capacity = n;
    }

    template<typename U>
    void Push(U&& p_element)
    {
        unique_lock<mutex> l_lock(m_rwMutex);
        m_cond.wait(l_lock, [this]() -> bool { return this->m_dq.size() < this->capacity; });
        m_dq.push_back(std::forward<T>(p_element));
        m_cond.notify_one();
    }

    T Pop()
    {
        try
        {
            unique_lock<mutex> l_lock(m_rwMutex);
            m_cond.wait(l_lock, [this]() -> bool { return !this->m_dq.empty(); });
            T l_element = std::move(m_dq.front()); m_dq.pop_front();
            m_cond.notify_one();

            return l_element;
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';

            throw;
        }
    }

    int Size()
    {
        unique_lock<mutex> l_lock(m_rwMutex);
        return m_dq.size();
    }
};

void Thread1(BoundedBlockingQueue<int>& bq)
{
    int n = 0;

    while (n < 10)
    {
        auto element = bq.Pop();
        cout<<" here in consumer with element "<<element<<endl;
        n++;
    }
}

void Thread2(BoundedBlockingQueue<int>& bq)
{
    int n = 0;

    while (n < 10)
    {
        // std::this_thread::sleep_for(chrono::milliseconds(1000));
        bq.Push(n);
        n++;
    }
}

int main()
{
    BoundedBlockingQueue<int> bq(5);
    std::thread t1(Thread1, std::ref(bq));
    std::thread t2(Thread2, std::ref(bq));

    t1.join();
    t2.join();

    cout<<" end here "<<endl;
}