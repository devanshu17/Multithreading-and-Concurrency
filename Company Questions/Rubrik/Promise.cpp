#include <bits/stdc++.h>
using namespace std;

template <typename T>
class Promise
{
    bool isValueReady;
    bool isException;
    string exceptionMessage;
    mutex m;
    condition_variable m_cond;

    T val;

    public:

    Promise()
    {
        isValueReady = false;
        isException = false;
        exceptionMessage = "";
    }

    T Get()
    {
        unique_lock<mutex> l_lock(m);
        m_cond.wait(l_lock, [this]() -> bool { return this->isValueReady || this->isException; });

        if (isException)
        {
            throw std::runtime_error(exceptionMessage);
        }

        return val;
    }

    void Set(T value)
    {
        unique_lock<mutex> l_lock(m);
        val = value;
        isValueReady = true;
        m_cond.notify_one();
    }

    void SetException(string& exceptionMessage)
    {
        unique_lock<mutex> l_lock(m);
        isException = true;
        this->exceptionMessage = exceptionMessage;
        m_cond.notify_one();
    }
};

int main()
{

Promise<int> promise;

    // Start a new thread that will set the value of the promise after a delay
    std::thread t([&]() -> void {
        std::this_thread::sleep_for(std::chrono::seconds(6));
        int value = 42;
        promise.Set(value);
    });

    // In the main thread, get the value of the promise
    try {
        int value = promise.Get();
        std::cout << "Got value from promise: " << value << std::endl;
    } catch(const std::exception& e) {
        std::cerr << "Caught exception: " << e.what() << '\n';
    }

    // Join the thread
    t.join();

    return 0;
}