#include <thread>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <queue>
#include <chrono>
#include <bits/stdc++.h>

using namespace std;

template<typename T>
class RateLimiter{
    private:
        struct Compare{
            bool operator()(const pair<chrono::steady_clock::time_point, function<void()>> &a,
                            const pair<chrono::steady_clock::time_point, function<void()>> &b) const
            {
                return a.first > b.first;
            }
        };
        
        mutex mtx;
        condition_variable cv;
        priority_queue<pair<chrono::steady_clock::time_point,T>, 
                       vector<pair<chrono::steady_clock::time_point,T>>,
                       Compare> requestQueue;
        thread garbageCollector;
        int size;
        bool stop;
        vector<thread> workers;
        int requestWaiting;
        
    public:
        int count;
        RateLimiter(int n) : size(n), count(0), stop(false), requestWaiting(0) {
            for(int i=0;i<10;i++) // pool
            {
                thread worker([&](){
                    while(!stop)
                    {
                        unique_lock<mutex> lock(mtx);
                        cv.wait(lock,[this](){ return stop || !requestQueue.empty();});
                        
                        if(stop && requestQueue.empty())
                            return;
                        
                        if(!requestQueue.empty())
                        {
                            auto task = requestQueue.top();
                            // Check for expired jobs before executing
                            if(task.first<=chrono::steady_clock::now())
                            {
                                requestQueue.pop();
                                continue;
                            }
                            requestQueue.pop();
                            count++;
                            lock.unlock();
                            
                            try{
                                task.second();
                            }
                            catch(const exception e){
                                cerr << e.what() << '\n';
                            }
                            catch(...)
                            {
                                cerr << "unknown exception in user job " << endl;
                            }
                        }
                    }
                });
                
                workers.emplace_back(move(worker));
            }
            
            garbageCollector = thread([&](){
                while(true)
                {
                    unique_lock<mutex> lock(mtx);
                    while(!requestQueue.empty() && requestQueue.top().first < chrono::steady_clock::now())
                    {
                        requestQueue.pop();
                    }
                    
                    cv.notify_all();
                    lock.unlock();
                    
                    this_thread::sleep_for(chrono::seconds(2));
                    if(stop && requestQueue.empty())
                        return;
                }
            });
        }
        
        void addJob(T&& job, int expiryInSec)
        {
            unique_lock<mutex> lock(mtx);
            requestWaiting++;
            cv.wait(lock,[this](){ return requestQueue.size() < size;});
            requestWaiting--;
            requestQueue.emplace(pair(chrono::steady_clock::now() + chrono::seconds(expiryInSec), forward<T>(job)));
            cv.notify_one();
        }
        
        ~RateLimiter()
        {
            unique_lock<mutex> lock(mtx);
            stop = true;
            lock.unlock();
            cv.notify_all();
            for(auto &worker:workers)
            {
                if(worker.joinable())
                {
                    worker.join();
                }
            }
            
            if(garbageCollector.joinable())
            {
                garbageCollector.join();
            }
            
        }
        
        void WaitForAllJobs()
        {
            unique_lock<mutex> lock(mtx);
            cv.wait(lock,[this](){ return requestQueue.empty() && requestWaiting==0;});
        }
        
};

int main(){
    RateLimiter<function<void()>> limiter(30);
    
    for(int i=0;i<300;i++)
    {
        limiter.addJob([i](){
            this_thread::sleep_for(chrono::seconds(4));
            cout << "Job " << i << " completed after sleeping for 4 sec" << endl;
        },5);
    }
    
    limiter.WaitForAllJobs();
    
    cout << "count" << limiter.count << endl;
    return 0;
}
