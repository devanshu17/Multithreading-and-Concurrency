#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <condition_variable>
#include <bits/stdc++.h>
#include <semaphore>

using namespace std;

class UnisexBathroom{
    private:
        mutex mtx;
        condition_variable cv;
        int empInBathroom;
        string isUseBy;
        counting_semaphore<3> maxEmps;
        
        static const string WOMEN;
        static const string MEN;
        static const string NONE;

        void useBathroom(const string &Name)
        {
            cout << Name << " using bathroom. Current emp in bathroom " << empInBathroom << endl;
            this_thread::sleep_for(chrono::seconds(5));
            cout << Name << " is done using the bathroom" << endl;
        }

    public:
        UnisexBathroom() : isUseBy(NONE), empInBathroom(0), maxEmps(3) {}
        
        void useBathroomMen(const string &name)
        {
            maxEmps.acquire();
            unique_lock<mutex> lock(mtx);
            cv.wait(lock,[this](){return isUseBy == NONE || isUseBy == MEN;});
            
            empInBathroom++;
            isUseBy = MEN;
            
            lock.unlock(); // unlock while using the bathroom to allow other threads to proceed
            
            useBathroom(name);
            lock.lock();
            
            empInBathroom--;
            if(empInBathroom==0) isUseBy = NONE;
            maxEmps.release();
            cv.notify_all();
        }
        
        void useBathroomWomen(const string &name)
        {
            unique_lock<mutex> lock(mtx);
            cv.wait(lock,[this](){return isUseBy == NONE || isUseBy == WOMEN;});
            empInBathroom++;
            isUseBy = WOMEN;
            maxEmps.acquire();
            
            lock.unlock(); // unlock while using the bathroom to allow other threads to proceed
            useBathroom(name);
            lock.lock();
            
            empInBathroom--;
            if(empInBathroom==0) isUseBy = NONE;
            maxEmps.release();
            cv.notify_all();
        }
            static void runTest(){
        UnisexBathroom unisexBathroom;
        
        auto female1 = thread([&unisexBathroom]{
            unisexBathroom.useBathroomWomen("Lisa");
        });
        
        auto male1 = thread([&unisexBathroom]{
            unisexBathroom.useBathroomMen("Bob");
        });
        
        auto male2 = thread([&unisexBathroom]{
            unisexBathroom.useBathroomMen("Anil");
        });
        
        auto male3 = thread([&unisexBathroom]{
            unisexBathroom.useBathroomMen("Dev");
        });
        
        auto male4 = thread([&unisexBathroom]{
            unisexBathroom.useBathroomMen("Rohit");
        });
        
        female1.join();
        male1.join();
        male2.join();
        male3.join();
        male4.join();
    }
};

void menQueue(UnisexBathroom &bathroom, string name)
{
    while (true)
    {
        bathroom.useBathroomMen(name);
        this_thread::sleep_for(chrono::seconds(1));
    }

}

void womenQueue(UnisexBathroom &bathroom, string name)
{
    while (true)
    {
        bathroom.useBathroomWomen(name);
        this_thread::sleep_for(chrono::seconds(1));
    }
}

const  string UnisexBathroom::NONE = "none"; 
const  string UnisexBathroom::MEN = "men"; 
const  string UnisexBathroom::WOMEN = "women"; 


int main()
{
    // UnisexBathroom::runTest();
    UnisexBathroom bathroom;
    vector<thread> threads;



    for(int i=0; i<5; i++)
    {
        threads.emplace_back(womenQueue, ref(bathroom), "W"+to_string(i));
    }
    
    for(int i=0; i<5; i++)
    {
        threads.emplace_back(menQueue, ref(bathroom), "M"+to_string(i));
    }
    
    for(auto &t:threads)
        t.join();
    return 0;
}
