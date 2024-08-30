#include <stdio.h>
#include <bits/stdc++.h>
#include <mutex>
#include <iostream>
#include <shared_mutex>
#include <ctime>
#include <chrono>

using namespace std;

class RequestTracker{
    private:
        unordered_map<string,deque<pair<time_t,int>>> requests;
        unordered_map<string,shared_mutex> ipMutexMap;
        unordered_map<string,time_t> lastAccess;
        shared_mutex mutexMapLock;
        thread cleanupThread;
        int cleanupInterval;
        bool stopCleanup = false;
        int timeWindow;
        // mutex mtx;
        
        void cleanOldRequests(const string &ip,time_t curTime)
        {
            while(!requests[ip].empty() && curTime - requests[ip].front().first > timeWindow)
                requests[ip].pop_front();
        }
        
        shared_mutex& getMutex(const string &ip)
        {
            {
                shared_lock<shared_mutex> mapLock(mutexMapLock);
                if(ipMutexMap.find(ip)!=ipMutexMap.end())
                    return ipMutexMap[ip];
            }
            unique_lock<shared_mutex> uniqueMapLock(mutexMapLock);
            return ipMutexMap[ip];
        }
        
        void cleanupTask(){
            while(!stopCleanup)
            {
                this_thread::sleep_for(chrono::seconds(cleanupInterval));
                time_t curTime = chrono::system_clock::to_time_t(chrono::system_clock::now());
                
                unique_lock<shared_mutex> mapLock(mutexMapLock);
                for(auto it=lastAccess.begin();it!=lastAccess.end();)
                {
                    if(curTime - it->second > timeWindow)   
                    {
                        requests.erase(it->first);
                        ipMutexMap.erase(it->first);
                        it = lastAccess.erase(it);
                    }
                    else
                    {
                        it++;
                    }
                }
            }
        }
        
    public:
        RequestTracker(int timeWindow, int cleanupInterval) : timeWindow(timeWindow), cleanupInterval(cleanupInterval) {
            cleanupThread = thread(&RequestTracker::cleanupTask,this);
        }
        
        ~RequestTracker()
        {
            stopCleanup = true;
            if(cleanupThread.joinable())
                cleanupThread.join();
        }
        
        void addRequests(const string &ip)
        {
            // lock_guard<mutex> lock(mtx);
            unique_lock<shared_mutex> lock(getMutex(ip));
            time_t curTime = chrono::system_clock::to_time_t(chrono::system_clock::now());
            
            cleanOldRequests(ip,curTime);
            
            if(requests[ip].empty() || requests[ip].back().first!=curTime)
                requests[ip].emplace_back(curTime,1);
            else
            {
                requests[ip].back().second++;
            }
            lastAccess[ip] = curTime;
        }
        
        int getRequestCount(const string &ip)
        {
            // lock_guard<mutex> lock(mtx);
            unique_lock<shared_mutex> lock(getMutex(ip));
            time_t curTime = chrono::system_clock::to_time_t(chrono::system_clock::now());
            
            cleanOldRequests(ip,curTime);
            
            if(requests.find(ip)==requests.end() || requests[ip].empty())
                return 0;
            
            lastAccess[ip] = curTime;
            int count = 0;
            for(const auto &entry:requests[ip])
            {
                count += entry.second;
            }
            return count;
        }
};
int main()
{
    RequestTracker tracker(60,10);
    
    tracker.addRequests("192.168.1.1");
    tracker.addRequests("192.168.1.1");
    
    cout << "Requests for 192.168.1.1: " << tracker.getRequestCount("192.168.1.1") << endl;
    
    this_thread::sleep_for(chrono::seconds(65));  // Wait long enough for cleanup

    tracker.addRequests("192.168.1.2");

    cout << "Requests from 192.168.1.1 after cleanup: " << tracker.getRequestCount("192.168.1.1") << endl;
    cout << "Requests from 192.168.1.2: " << tracker.getRequestCount("192.168.1.2") << endl;

    return 0;
}