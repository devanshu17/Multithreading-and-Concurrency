#include <iostream>
#include <bits/stdc++.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

using namespace std;

class ReaderWriterLock{
    public:
        ReaderWriterLock() : activeReaders(0), isWriteActive(false) {};
        
        void lock_read()
        {
            unique_lock<mutex> lock(mtx);
            cv_writer.wait(lock,[this](){ return !isWriteActive; });
            
            activeReaders++;
            
            cv_reader.notify_all();
        }
        
        void unlock_read(){
            unique_lock<mutex> lock(mtx);
            
            if(--activeReaders==0)
                cv_writer.notify_one();
        }
        
        void lock_write()
        {
            unique_lock<mutex> lock(mtx);
            cv_writer.wait(lock,[this](){ return !isWriteActive && activeReaders==0; });
            
            isWriteActive = true;
        }
        
        void unlock_write()
        {
            unique_lock<mutex> lock(mutex);
            
            isWriteActive = false;
            
            cv_reader.notify_all();
            cv_writer.notify_all();
        }
        
    private:
        mutex mtx;
        condition_variable cv_reader;
        condition_variable cv_writer;
        int activeReaders;
        bool isWriteActive;
};

int shared_data = 0;

void reader(int id, ReaderWriterLock &rwLock)
{
    while(true)
    {
        rwLock.lock_read();
        cout << "Reader " << id << " read value " << shared_data << endl;
        rwLock.unlock_read();
        
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

void writer(int id, ReaderWriterLock &rwLock)
{
    while(true)
    {
        rwLock.lock_write();
        shared_data++;
        cout << "Writer " << id << " updated the value to: " << shared_data << endl;
        rwLock.unlock_write();
        this_thread::sleep_for(chrono::milliseconds(500));
    }
}
int main()
{
    std::cout<<"Hello World"<< endl;
    ReaderWriterLock rwLock;
    
    vector<thread> threads;
    
    for(int i=0;i<5;i++)
        threads.emplace_back(reader,i,ref(rwLock));
    
    for(int i=0;i<1;i++)
        threads.emplace_back(writer,i,ref(rwLock));
    
    for(auto &t:threads)
        t.join();

    return 0;
}