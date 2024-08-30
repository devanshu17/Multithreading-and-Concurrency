#include <thread>
#include <condition_variable>
#include <mutex>
#include <functional>
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
using namespace std;

template <typename T>
class ClientWrapper
{
    bool isConnected;
    int requestServing;
    int initInProgress;
    condition_variable cv;
    bool connectionEnded;
    mutex m;

    public:
    ClientWrapper (const ClientWrapper&) = delete;
    ClientWrapper operator=(const ClientWrapper&) = delete;

    ClientWrapper()
    {
        isConnected = false;
        requestServing = 0;
        initInProgress = 0;
        connectionEnded = false;
    }

    void Init(T&& initialize)
    {
        unique_lock<mutex> l_lock(m);
        initInProgress++;

        try
        {
            initialize();
        }
        catch(const std::exception& e)
        {
            std::cout << e.what() << '\n';
            initInProgress--;
            cv.notify_all();
            throw e;
        }
        
        initInProgress--;
        isConnected = true;
        connectionEnded = false;
        cv.notify_all();
    }

    void Request(T&& req)
    {
        unique_lock<mutex> l_lock(m);
        cv.wait(l_lock, [this]() -> bool { return (isConnected && this->initInProgress == 0) || this->connectionEnded; });

        if (connectionEnded)
        {
            cout<<"returned as no connection"<<endl;
            return;
        }

        requestServing++;
        l_lock.unlock();

        try
        {
            req();
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            l_lock.lock();
            requestServing--;
            l_lock.unlock();

            throw e;
        }
        

        l_lock.lock();
        requestServing--;
        l_lock.unlock();
        cv.notify_all();
    }

    void Close(T&& close)
    {
        unique_lock<mutex> l_lock(m);
        cv.wait(l_lock, [this]() -> bool { return (isConnected && this->initInProgress == 0 && this->requestServing == 0) || this->connectionEnded; });

        if (connectionEnded)
        {
            cout<<"returned as no connection"<<endl;
            return;
        }

        try
        {
            close();
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            throw e;
        }

        isConnected = false;
        connectionEnded = true;
        cv.notify_all();
    }
};

int main()
{
    // Create a ClientWrapper object
    ClientWrapper<function<void()>> client;

    // Create a vector of function pointers to the ClientWrapper methods
    std::vector<std::function<void()>> functions = {
        [&]() { client.Init([]() { std::cout << "Initializing\n"; }); },
        [&]() { client.Request([]() { std::cout << "Requesting\n"; }); },
        [&]() { client.Close([]() { std::cout << "Closing\n"; }); }
    };

    // Create a random engine
    std::default_random_engine engine(std::random_device{}());

    // Create a vector of threads
    std::vector<std::thread> threads;

    // Make 1000 calls to each function in random order
    for (int i = 0; i < 1000; ++i) {
        // Shuffle the functions
        std::shuffle(functions.begin(), functions.end(), engine);

        // Call each function in the shuffled order
        for (auto& function : functions) {
            threads.push_back(std::thread(function));
        }
    }

    // Join all threads
    for (auto& thread : threads) {
        thread.join();
    }

    return 0;
}