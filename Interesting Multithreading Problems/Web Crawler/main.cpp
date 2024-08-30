#include <stdio.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <bits/stdc++.h>

using namespace std;

class HTMLParser{
    public:
        vector<string> getUrls(string url){
            return {"http://news.yahoo.com", "http://news.yahoo.com/news", "http://news.yahoo.com/news/topics/",
                "http://news.google.com", "http://news.yahoo.com/us", "http://news.google.com/news", "http://news.google.com/us"};
        }
};

class WebCrawler{
    public:
        vector<string> crawl(string startUrl, HTMLParser &htmlParser)
        {
            hostName = extractHostName(startUrl);
            urlQueue.push(startUrl);
            visited.insert(startUrl);
            
            vector<thread> threads;
            
            for(int i=0;i<thread::hardware_concurrency();i++)
            {
                threads.emplace_back(&WebCrawler::crawlHelper,this,ref(htmlParser));
            }
            
            // In our crawler, we want to ensure all crawling is done before returning results, which aligns with join().
            for(auto &t : threads )
            {
                t.join();
            }
            
            return vector<string>(visited.begin(), visited.end());
        }
        
    private:
        queue<string> urlQueue;
        unordered_set<string> visited;
        mutex mtx;
        condition_variable cv;
        int activeThreads = 0;
        string hostName;
        
        void crawlHelper(HTMLParser &htmlParser)
        {
            cout << this_thread::get_id() << endl;
            while(true)
            {
                string visitedUrl;
                {   
                    unique_lock<mutex> lock(mtx);
                    cv.wait(lock,[this](){ return !urlQueue.empty() || activeThreads==0; });
                    
                    if(urlQueue.empty() && activeThreads==0)
                        return;
                    
                    if(urlQueue.empty())
                        continue;
                    
                    visitedUrl = urlQueue.front();
                    urlQueue.pop();
                    activeThreads++;
                }
                
                vector<string> newUrls = htmlParser.getUrls(visitedUrl);
                
                {
                    lock_guard<mutex> lock(mtx);
                    for(const string& newUrl : newUrls)
                    {
                        if(extractHostName(newUrl)==hostName && visited.find(newUrl)==visited.end())
                        {
                            urlQueue.push(newUrl);
                            visited.insert(newUrl);
                        }
                    }
                    activeThreads--;
                }
                cv.notify_all();
            }
        }
        
        string extractHostName(const string &url)
        {
            int start = url.find("//") + 2;
            int end = url.find('/',start);
            return url.substr(start,end-start);
            // if(url.size()==0)  
            //     return "";
            // return url.substr(0,url.find('/',7));
        }
};

class WebCrawler2{
    
    public:
        vector<string> crawler(string startUrl,HTMLParser &htmlParser)
        {
            queue<string> urlQueue;
            unordered_set<string> visUrl;
            vector<string> res;
            
            int activeThreads = 0;            
            mutex mtx;
            condition_variable cv;
            
            urlQueue.push(startUrl);
            visUrl.insert(startUrl);
            
            //The web crawler's main loop spawns new threads to fetch URLs from the web pages. If join() were used, 
            //the main loop would block and wait for each thread to finish before continuing to spawn new threads. 
            
            while(!urlQueue.empty() || activeThreads>0)
            {
                cout << this_thread::get_id() << " AT : " << activeThreads << " QS: " << urlQueue.size() << endl;
                unique_lock<mutex> lock(mtx);
                // counter like activeThreads and a condition_variable can ensure that the main program waits for all threads to finish their work before exiting. 
                // This prevents the program from ending prematurely while detached threads are still running.
                cv.wait(lock,[&](){ return !urlQueue.empty() || activeThreads==0;});
                
                if(!urlQueue.empty())
                {
                    string url = urlQueue.front();
                    res.push_back(url);

                    urlQueue.pop();
                    // Incrementing activeThreads before the thread starts its work ensures that the main loop is aware 
                    // that there is a new active task (a new thread fetching URLs). This prevents the main loop from 
                    // terminating prematurely because it knows that there are still ongoing operations.
                    activeThreads++; 
                    cout << "Inc activeThreads after adding " << " " << url << " " << this_thread::get_id() << endl;
                    
                    lock.unlock();
                    cv.notify_all();
                    
                    thread([&,url](){
                        cout << "Spawned: " << this_thread::get_id() << " AT : " << activeThreads << " QS: " << urlQueue.size() << endl;
                        vector<string> urls = htmlParser.getUrls(url);
                        
                        lock_guard<mutex> guard(mtx);
                        for(auto& newUrl : urls)
                        {
                            if(visUrl.find(newUrl)==visUrl.end() && getHostName(newUrl) == getHostName(startUrl))
                            {
                                visUrl.insert(newUrl);
                                urlQueue.push(newUrl);
                            }
                        }
                        activeThreads--;
                        cout << "Dec activeThreads" << " " << this_thread::get_id() << endl;
                        cv.notify_all();
                    }).detach(); //By using detach(), each thread can run independently in the background, allowing the main loop to continue spawning new threads for other URLs.
                    // detach() is appropriate here because the main loop's responsibility is to manage the overall flow of URL fetching, not to sequentially process each URL. 
                    // Detached threads allow for maximum concurrency, ensuring that URLs are fetched as soon as possible, without unnecessary waiting.
                }
            }
            return res;
        }
    private:
        string getHostName(string &url)
        {
            return url.substr(0,url.find('/',7));
        }
};


int main()
{
    WebCrawler crawler;
    WebCrawler2 crawler2;
    HTMLParser parser;
    
    auto start = chrono::steady_clock::now();
    vector<string> res = crawler.crawl("http://news.yahoo.com", parser);
    auto end = chrono::steady_clock::now();
    chrono::duration<double> elapsed = end - start;
    
    cout << "--------Time 1 : " << elapsed.count() << "------------" << endl;
    
    for(const auto &url : res)
        cout << url << endl;
        
    cout << endl;
    cout << "Using WebCrawler2" << endl;
    
    start = chrono::steady_clock::now();
    vector<string> res2 = crawler2.crawler("http://news.google.com",parser);
    end = chrono::steady_clock::now();
    elapsed = end - start;
    cout << "--------Time 2 : " << elapsed.count() << "------------" << endl;
    
    for(const auto &url : res2)
        cout << url << endl;
        
    return 0;

}