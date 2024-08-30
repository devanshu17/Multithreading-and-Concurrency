#include <stdio.h>
#include <iostream>
#include <mutex>
#include <bits/stdc++.h>
#include <thread>

using namespace std;

class BlockingQueue{
    public:
        explicit BlockingQueue(int cap) : capacity(cap){}
        
        void enqueue(int num)
        {
            unique_lock<mutex> lock(m);
            cv_full.wait(lock,[this] { return q.size()<capacity;});
            
            q.push(move(num));
            
            cv_empty.notify_one();
        }
        
        int dequeue()
        {
            unique_lock<mutex> lock(m);
            cv_empty.wait(lock,[this]{return !q.empty();});
            int item = move(q.front());
            q.pop();
            cv_full.notify_one();
            
            return item;
        }
    
    private:
        queue<int> q;
        mutex m;
        condition_variable cv_full;
        condition_variable cv_empty;
        int capacity;
        
};

void producer(BlockingQueue &bq)
{
    for(int i=0;i<100;i++)
    {
        cout << "Producing: " << i+1 << endl;
        bq.enqueue(i+1);
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

void consumer(BlockingQueue &bq)
{
    for(int i=0;i<100;i++)
    {
        int item = bq.dequeue();
        cout << "Consuming: " << item << endl;
        this_thread::sleep_for(chrono::milliseconds(150));

    }
}

int main()
{
    printf("Hello World");
    BlockingQueue bq(3);
    // thread t1(&BlockingQueue::enqueue,&bq,10);
    // thread t2(&BlockingQueue::dequeue,ref(bq));
    thread t1(producer,ref(bq));
    thread t2(consumer,ref(bq));
    
    t1.join();
    t2.join();

    return 0;
}