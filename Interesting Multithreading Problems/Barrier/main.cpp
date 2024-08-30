
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <bits/stdc++.h>

using namespace std;

class Barrier{
    public:
        Barrier(int size) : capacity(size) , count(0), barrierCycle(0), released(0) {} 
        
        void await(){
            unique_lock<mutex> lock(mtx);
            count++;
            int cycle = barrierCycle;
            
            if(count == capacity)
            {
                count = 0;
                barrierCycle++;
                cv.notify_all();
            }
            else
            {
                cv.wait(lock,[this,cycle]{ return cycle!=barrierCycle;});
            }
        }
        
    private:
        int capacity;
        int barrierCycle;
        int count;
        mutex mtx;
        condition_variable cv;
        int released;
};

void thread_function(Barrier &b,int id, int delay)
{
    this_thread::sleep_for(chrono::milliseconds(delay));
    cout << "Thread " << id << endl;
    b.await();
    
    this_thread::sleep_for(chrono::milliseconds(delay));
    cout << "Thread " << id<< id  << endl;
    b.await();
    
    this_thread::sleep_for(chrono::milliseconds(delay));
    cout << "Thread " << id <<id<<id<< endl;
    b.await();

}

int main()
{
    Barrier b(3);
    thread t1(thread_function,ref(b),1,0);
    thread t2(thread_function,ref(b),2,500);
    thread t3(thread_function,ref(b),3,1000);
    
    t1.join();
    t2.join();
    t3.join();

    return 0;
}