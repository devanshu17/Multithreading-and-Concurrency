#include <thread>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <queue>
#include <atomic>

template <typename T>
class RateLimiter
{
    int size;

    struct Compare {
        bool operator()(const std::pair<std::chrono::steady_clock::time_point, std::function<void()>>& a, 
                        const std::pair<std::chrono::steady_clock::time_point, std::function<void()>>& b) const 
        {
            return a.first > b.first;
        }
    };

    std::priority_queue<std::pair<std::chrono::steady_clock::time_point, T>,
        std::vector<std::pair<std::chrono::steady_clock::time_point, T>>,
        Compare> m_requests;

    std::vector<std::thread> m_workers;
    std::atomic<bool> m_stop;
    std::condition_variable m_cv;
    std::mutex m_mutex;
    std::thread m_garbageCollector;
    int requestWaiting;

    public:
    int count;

    RateLimiter(int n)
    {
        count = 0;
        requestWaiting = 0;
        size = n;
        m_stop.store(false);

        for (int i = 0; i < 10; i++)
        {
            std::thread worker([&]() -> void
            {
                while (!m_stop.load())
                {
                    std::unique_lock<std::mutex> l_lock(m_mutex);
                    m_cv.wait(l_lock, [&]() -> bool { return m_stop.load() || !m_requests.empty(); });

                    if (m_stop.load())
                    {
                        return;
                    }

                    auto work = m_requests.top(); m_requests.pop();
                    count++;
                    l_lock.unlock();
                    m_cv.notify_all();
                    
                    try
                    {
                        work.second();
                    }
                    catch(const std::exception& e)
                    {
                        std::cerr << e.what() << '\n';
                    }
                    catch (...)
                    {
                        std::cerr << " unknown exception in user job " << std::endl;
                    }
                }
            });

            m_workers.push_back(std::move(worker));
        }
    
        m_garbageCollector = std::thread([&]() -> void {

            while (!m_stop.load())
            {
                std::unique_lock<std::mutex> l_lock(m_mutex);
                
                while (m_requests.size() && m_requests.top().first < std::chrono::steady_clock::now())
                {
                    m_requests.pop();
                }

                m_cv.notify_all();
                l_lock.unlock();
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        });
    }

    void Add(T&& p_job, int expiryInSecs)
    {
        requestWaiting++;

        std::unique_lock<std::mutex> l_lock(m_mutex);
        m_cv.wait(l_lock, [&]() -> bool { return this->m_requests.size() < size; });
        requestWaiting--;
        m_requests.push(std::pair(std::chrono::steady_clock::now() + std::chrono::seconds(expiryInSecs), std::forward<T>(p_job)));
        m_cv.notify_one();
    }

    ~RateLimiter()
    {
        m_stop.store(true);
        m_cv.notify_all();

        for (auto& worker : m_workers)
        {
            if (worker.joinable())
            {
                worker.join();
            }
        }

        if (m_garbageCollector.joinable())
        {
            m_garbageCollector.join();
        }
    }

    void WaitForAllJobs() 
    {
        std::unique_lock<std::mutex> l_lock(m_mutex);
        m_cv.wait(l_lock, [this]() -> bool { return m_requests.empty() && requestWaiting == 0; });
    }
};

int main() {
    // Create a RateLimiter with 10 worker threads
    RateLimiter<std::function<void()>> limiter(30);

    // Add 300 jobs to the limiter
    for (int i = 0; i < 300; ++i) {
        // Each job is a lambda function that sleeps for 3 seconds
        limiter.Add([i]() -> void {
            std::this_thread::sleep_for(std::chrono::seconds(4));
            std::cout << "Job " << i << " completed after sleeping for 4 seconds.\n";
        }, 2); // The jobs are added without any waiting time
    }

    limiter.WaitForAllJobs();
    std::cout << "count " << limiter.count<< std::endl;
    // The limiter will automatically clean up the worker threads when it goes out of scope
    return 0;
}