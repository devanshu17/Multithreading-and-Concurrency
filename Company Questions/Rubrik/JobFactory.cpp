#include <thread>
#include <condition_variable>
#include <queue>
#include <functional>
#include <random>
#include <iostream>
#include <atomic>

template <typename T>
class JobFactory
{
    std::queue<std::pair<T, std::chrono::steady_clock::time_point>> m_jobs;
    int m_worker_count;
    std::vector<std::thread> m_workers;
    std::atomic<bool> m_stop;
    std::condition_variable m_cv;
    std::mutex m_mutex;

    public:
    JobFactory(int n)
    {
        m_worker_count = n;
        m_stop.store(false);

        for (int i = 0; i < n; i++)
        {
            std::thread worker([&]() -> void
            {
                while (!m_stop.load())
                {
                    std::unique_lock<std::mutex> l_lock(m_mutex);
                    m_cv.wait(l_lock, [&]() -> bool { return m_stop.load() || !m_jobs.empty(); });

                    if (m_stop.load())
                    {
                        return;
                    }

                    auto work = m_jobs.front(); m_jobs.pop();
                    l_lock.unlock();
                    m_cv.notify_all();

                    if (work.second < std::chrono::steady_clock::now())
                    {
                        std::this_thread::sleep_until(work.second);
                    }

                    try
                    {
                        work.first();
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
    }

    void AddJob(T&& p_job, int waitingTimeInSecs)
    {
        std::unique_lock<std::mutex> l_lock(m_mutex);
        m_jobs.push(std::pair(std::forward<T>(p_job), std::chrono::steady_clock::now() + std::chrono::seconds(waitingTimeInSecs)));
        m_cv.notify_one();
    }

    ~JobFactory()
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
    }

    void WaitForAllJobs() 
    {
        std::unique_lock<std::mutex> l_lock(m_mutex);
        m_cv.wait(l_lock, [this]() -> bool { return m_jobs.empty(); });
    }
};

int main() {
    JobFactory<std::function<void()>> factory(1000);

    // Create a random number generator
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(1,2);

    // Add 10000 jobs to the factory
    for (int i = 0; i < 10000; ++i) {
        // Each job is a lambda function that sleeps for a random duration
        factory.AddJob([i, &distribution, &generator] {
            int sleepTime = distribution(generator);
            std::this_thread::sleep_for(std::chrono::seconds(sleepTime));
            std::cout << "Job " << i << " completed after sleeping for " << sleepTime << " seconds.\n";
        }, 2); // The jobs are added without any waiting time
    }

    factory.WaitForAllJobs();
    // The factory will automatically clean up the worker threads when it goes out of scope
    return 0;
}