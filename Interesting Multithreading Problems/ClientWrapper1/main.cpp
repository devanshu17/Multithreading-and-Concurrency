/*

Class ClientWrapper(Client):
init()
request()
close()

Here, class Client is like an abstract class that deals with responsibilities like initializing connection to the server using init(), request() is used to make request to the server and close() is used to close the connection to the server.
Question: Multiple threads can concurrently access methods of class ClientWrapper. One needs to implement all the methods of class ClientWrapper such that following use cases are covered -

If init() is in progress by some thread, then block other concurrent threads to execute request() or close() method
Do not allow concurrent calls to init() method.
Allow concurrent calls to request() method if init() is already called successfully.
If some thread is still using request(), then block any other thread to close the connection using close() method
Once connection is closed, do not allow subsequent calls to close() method
Do not allow concurrent calls to close() method


*/

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std;

class Client {
public:
    virtual void init() {
        cout << "Initializing connection..." << endl;
        this_thread::sleep_for(chrono::seconds(1)); // Simulate initialization delay
        cout << "Connection initialized." << endl;
    }

    virtual void request() {
        cout << "Making request..." << endl;
        this_thread::sleep_for(chrono::seconds(1)); // Simulate request processing
        cout << "Request processed." << endl;
    }

    virtual void close() {
        cout << "Closing connection..." << endl;
        this_thread::sleep_for(chrono::seconds(1)); // Simulate closing delay
        cout << "Connection closed." << endl;
    }
};

class ClientWrapper : public Client {
private:
    mutex mtx;
    condition_variable cv;
    bool isInitialized = false;
    bool isClosed = false;
    int activeRequests = 0;
    bool initInProgress = false;
    bool closeInProgress = false;

public:
    void init() override {
        unique_lock<mutex> lock(mtx);

        // Block if another thread is already initializing
        if (initInProgress || isInitialized) {
            cv.wait(lock, [this]() { return !initInProgress && !isInitialized; });
        }

        // Mark init as in progress
        initInProgress = true;

        // Call the base class init method
        Client::init();

        // Mark initialization complete
        isInitialized = true;
        initInProgress = false;

        // Notify other threads waiting on this condition
        cv.notify_all();
    }

    void request() override {
        unique_lock<mutex> lock(mtx);

        // Block if not initialized or if close is in progress or finished
        cv.wait(lock, [this]() { return isInitialized && !initInProgress && !closeInProgress && !isClosed; });

        // Increase active request count
        activeRequests++;

        // Unlock mutex while making request to allow other requests to proceed
        lock.unlock();

        // Call the base class request method
        Client::request();

        // Lock mutex again to update state
        lock.lock();

        // Decrease active request count and notify if this was the last request
        activeRequests--;
        if (activeRequests == 0) {
            cv.notify_all();
        }
    }

    void close() override {
        unique_lock<mutex> lock(mtx);

        // Block if close is already in progress, or if there are active requests
        cv.wait(lock, [this]() { return !closeInProgress && (isInitialized && !initInProgress) && activeRequests == 0 && !isClosed; });

        // Mark close as in progress
        closeInProgress = true;

        // Call the base class close method
        Client::close();

        // Mark connection as closed
        isClosed = true;
        closeInProgress = false;
        isInitialized = false;

        // Notify all other threads that close is complete
        cv.notify_all();
    }
};

int main() {
    ClientWrapper client;

    // Threads to test concurrency
    thread t1([&]() { client.init(); });
    thread t2([&]() { client.request(); });
    thread t3([&]() { client.request(); });
    thread t6([&]() { client.init(); });
    thread t4([&]() { client.close(); });
    thread t5([&]() { client.request(); });

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    t6.join();
    
    

    return 0;
}
