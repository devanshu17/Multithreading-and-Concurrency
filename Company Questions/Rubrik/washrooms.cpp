#include <mutex>
#include <condition_variable>
#include <thread>
#include <iostream>
#include <vector>

class Washroom
{
    int m_currentGender;
    int m_curentUsers;
    int m_size;
    std::mutex m_mutex;
    std::condition_variable m_cv;  
    std::chrono::steady_clock::time_point m_genderAssigned;
    bool m_genderAlreadyAssigned;

    public:
    Washroom(int capacity)
    {
        m_size = capacity;
        m_curentUsers = 0;
        m_currentGender = 0;
        m_genderAlreadyAssigned = false;
    }

    void Enter(int p_gender)
    {
        std::unique_lock<std::mutex> l_lock(m_mutex);
        m_cv.wait(l_lock, [&]() -> bool {
            return this->m_curentUsers < this->m_size && (this->m_currentGender == 0 || p_gender == this->m_currentGender) &&
        (!this->m_genderAlreadyAssigned || std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - this->m_genderAssigned).count() < 10); });
        m_curentUsers++;
        m_currentGender = p_gender;

        if (!m_genderAlreadyAssigned)
        {
            m_genderAssigned = std::chrono::steady_clock::now();
            m_genderAlreadyAssigned = true;
        }
    }

    void Leave()
    {
        std::lock_guard<std::mutex> l_lock(m_mutex);
        m_curentUsers--;

        if (m_curentUsers == 0)
        {
            m_currentGender = 0;
            m_genderAssigned = std::chrono::steady_clock::now();
            m_genderAlreadyAssigned = false;
        }

        m_cv.notify_all();
    }

    bool isFree()
    {
        std::lock_guard<std::mutex> l_lock(m_mutex);
        return m_curentUsers == 0;
    }

    int GetType()
    {
        std::lock_guard<std::mutex> l_lock(m_mutex);
        return m_currentGender;
    }

    int GetSize()
    {
        std::lock_guard<std::mutex> l_lock(m_mutex);
        return m_curentUsers;
    }
};

bool ShouldAssign(int p_gender, Washroom& w)
{
    static std::mutex m;

    bool l_isFree = w.isFree();
    int l_type = w.GetType();

    {   
        std::lock_guard<std::mutex> l_lock(m);
        std::cout<<" washroom running at capacity " << w.GetSize() << std::endl;
    }

    return l_isFree || l_type == 0 || l_type == p_gender;
}

void Request(int p_gender, Washroom& w1, Washroom& w2)
{
    static std::mutex m;

    if (ShouldAssign(p_gender, w1))
    {
        w1.Enter(p_gender);
        {   
            std::lock_guard<std::mutex> l_lock(m);
            std::cout<<" gender " << p_gender << " started using washroom w1 "<< std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(3));
        w1.Leave();

        {   
            std::lock_guard<std::mutex> l_lock(m);
            std::cout<<" gender " << p_gender << " leaving from washroom w1 "<< std::endl;
        }
    }
    else if (ShouldAssign(p_gender, w2))
    {
        w2.Enter(p_gender);

        {   
            std::lock_guard<std::mutex> l_lock(m);
            std::cout<<" gender " << p_gender << " started using washroom w2 "<< std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(3));
        w2.Leave();

        {   
            std::lock_guard<std::mutex> l_lock(m);
            std::cout<<" gender " << p_gender << " leaving from washroom w2 "<< std::endl;
        }
    }
    else
    {
        auto& w = w1.GetSize() < w2.GetSize() ? w1 : w2;
        w.Enter(p_gender);
        {   
            std::lock_guard<std::mutex> l_lock(m);
            std::cout<<" gender " << p_gender << " started using washroom w2 "<< std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(3));
        w.Leave();

        {   
            std::lock_guard<std::mutex> l_lock(m);
            std::cout<<" gender " << p_gender << " leaving from washroom w "<< std::endl;
        }
    }
}

int main()
{
    Washroom washroom1(10);  // capacity of 10
    Washroom washroom2(10);  // capacity of 10

    std::vector<std::thread> maleThreads;
    std::vector<std::thread> femaleThreads;

    // Create 100 threads for males
    std::thread t1([&]() -> void {
        for (int i = 0; i < 100; ++i)
        {
            maleThreads.push_back(std::thread(Request, 1, std::ref(washroom1), std::ref(washroom2)));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));  // slight delay between each request
        }
    });

    // Create 100 threads for females
    std::thread t2([&]() -> void {
        for (int i = 0; i < 100; ++i)
        {
            femaleThreads.push_back(std::thread(Request, 2, std::ref(washroom1), std::ref(washroom2)));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));  // slight delay between each reques
        }
    });

    t1.join();
    t2.join();
    // Join all male threads
    for (auto& thread : maleThreads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    // Join all female threads
    for (auto& thread : femaleThreads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    return 0;
}