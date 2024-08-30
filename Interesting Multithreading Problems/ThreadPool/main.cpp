#include <iostream>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <bits/stdc++.h>
#include <functional>

using namespace std;

class ThreadPool{
    public:
        ThreadPool(int numThreads) : totalThreads(numThreads), stop(false) {
            for(int i=0;i<numThreads;i++)
                workers.emplace_back(&ThreadPool::processQueue,this);
        }
        
        ~ThreadPool()
        {
            unique_lock<mutex> lock(mtx);
            stop = true;
            lock.unlock();
            cv.notify_all();
            for(auto &t:workers)
                t.join();
        }
        
        template<class F>
        void enqueue(F&& task) // or void enqueue(function<void()> &task)
        {
            unique_lock<mutex> lock(mtx);
            taskQueue.emplace(forward<F>(task)); // perfect forwarding of function, pass the parameter as it is
            lock.unlock();
            cv.notify_one();
        }
    private:
        queue<function<void()>> taskQueue;
        int totalThreads;
        mutex mtx;
        condition_variable cv;
        bool stop;
        vector<thread> workers;
        
        void processQueue()
        {
            while(true)
            {
                unique_lock<mutex> lock(mtx);
                cv.wait(lock,[this]{ return stop || !taskQueue.empty();});
                
                if(stop && taskQueue.empty())
                    return;
                    
                auto task = move(taskQueue.front());
                taskQueue.pop();
                lock.unlock();
                task();
            }
        }
};

int main()
{
    ThreadPool pool(3);
    
    for(int i=0;i<9;i++)
    {
        pool.enqueue([i]{
           cout << "Task " << i << " " << this_thread::get_id() << " executed by thread" << endl;
           this_thread::sleep_for(chrono::seconds(1));
        });
    }
    
    return 0;
}