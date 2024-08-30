#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <bits/stdc++.h>

using namespace std;

struct Task{
    int id;
    function<void()> taskFunction;
    vector<int> dependencies;
};

class Scheduler{
    public:
        Scheduler(vector<Task> &tasks) : tasks(tasks)
        {
            for(const auto &task : tasks)
            {
                taskMap[task.id] = task;
                if(task.dependencies.empty())
                    readyTaskQueue.push(task.id);
                else
                {
                    for(int dep:task.dependencies)
                    {
                        taskDepenciesCount[task.id]++;
                        depedents[dep].insert(task.id);
                    }
                }
            }
        }
        
        void run()
        {
            for(int i=0;i<thread::hardware_concurrency();i++)
            {
                workers.emplace_back([this]{
                   while(true)
                   {
                       unique_lock<mutex> lock(mtx);
                       cv.wait(lock,[this]{ return !readyTaskQueue.empty();});
                       
                    //   if(!readyTaskQueue.empty())
                    //   {
                           int executedTaskId = readyTaskQueue.front();
                           readyTaskQueue.pop();
                           
                           lock.unlock();
                           
                           taskMap[executedTaskId].taskFunction();
                           
                           lock.lock();
                           for(int dep:depedents[executedTaskId])
                           {
                               taskDepenciesCount[dep]--;
                               if(taskDepenciesCount[dep]==0)
                                   readyTaskQueue.push(dep);
                           }
                           cv.notify_all();
                    //   }
                       
                   }
                });
            }
            
            for(auto &worker: workers)
            {
                worker.join();
            }
        }
    
    private:
        vector<Task> tasks;
        unordered_map<int,Task> taskMap;
        unordered_map<int,int> taskDepenciesCount;
        unordered_map<int,unordered_set<int>> depedents;
        vector<thread> workers;
        queue<int> readyTaskQueue;
        mutex mtx;
        condition_variable cv;
};

int main(){
    vector<Task> tasks = {
      {1,[]{ cout << "TASK 1" << endl; }, {}},
      {2,[]{ cout << "TASK 2" << endl; }, {1}}, 
      {3,[]{ cout << "TASK 3" << endl; }, {1}}, 
      {4,[]{ cout << "TASK 4" << endl; }, {2,3}}, 
      {5,[]{ cout << "TASK 5" << endl; }, {1,3,4}}, 
      {6,[]{ cout << "TASK 6" << endl; }, {}},
      {7,[]{ cout << "TASK 7" << endl; }, {1,2,3,4,5,6}}, 
    };
    
    Scheduler scheduler(tasks);
    
    scheduler.run();
    
    return 0;
    
    
}