#include <iostream>
#include <thread>
#include <functional>
#include <mutex>
#include <bits/stdc++.h>
#include <condition_variable>
#include <chrono>

using namespace std;

class TaskScheduler{
    public: 
        void schedule(const function<void()> &task, chrono::milliseconds delay)
        {
            unique_lock<mutex> lock(mtx);
            auto timePoint = chrono::steady_clock::now() + delay;
            tasks.push({task,timePoint});
            cv.notify_one();
        }
        
        void run(){
            while(true){
                unique_lock<mutex> lock(mtx);
                cv.wait(lock,[this]{ return !tasks.empty();});
                
                auto now = chrono::steady_clock::now();
                if(tasks.top().taskTime < now)
                {
                    auto task = tasks.top().task;
                    tasks.pop();
                    lock.unlock();
                    task();
                }
                else
                {
                    cv.wait_until(lock,tasks.top().taskTime);
                }
            }
        }
    
    private:
        struct Task{
            function<void()> task;
            chrono::steady_clock::time_point taskTime;
            bool operator<(const Task& other) const
            {
                return taskTime > other.taskTime;
            }
        };
        priority_queue<Task> tasks;
        mutex mtx;
        condition_variable cv;
        
};
int main()
{
    std::cout<<"Hello World";
    TaskScheduler scheduler;
    
    for(int i=0;i<10;i++)
    {
        scheduler.schedule([i]{
            cout << "Task " << i << " executed my thread " << this_thread::get_id() << " at time : " <<  chrono::steady_clock::now().time_since_epoch().count() << endl;
        }, chrono::milliseconds(i*100));
    }
    
    thread schedulerThread([&scheduler] { 
        scheduler.run();
        });
        
    this_thread::sleep_for(chrono::seconds(2));

    schedulerThread.detach();
    
    

    return 0;
}