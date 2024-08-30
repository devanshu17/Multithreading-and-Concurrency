#include <thread>
#include <condition_variable>
#include <iostream>

class Uber
{
    int demoRequests, repulicRequests, demoSeated, republicSeated;
    std::condition_variable m_cv;
    std::mutex m_rideMutex;
    int peopleSeated;

    void Drive()
    {
        std::cout<<" car driven "<<std::endl;
    }

    void Seated()
    {
        std::cout<<" person seated "<<std::endl;
    }

    public:

    Uber()
    {
        demoRequests = 0;
        demoSeated = 0;
        republicSeated = 0;
        repulicRequests = 0;
        peopleSeated = 0;
    }

    void RequestDemo()
    {
        std::unique_lock<std::mutex> l_demoLock(m_rideMutex);
        demoRequests++;
        m_cv.wait(l_demoLock, [this]() -> bool { return this->demoRequests >= 4 || this->repulicRequests >= 4 || (this->demoRequests >= 2 && this->repulicRequests >= 2); });
        demoSeated++;
        Seated();
        peopleSeated++;

        if (peopleSeated == 4)
        {
            peopleSeated = 0;

            if (selectedCombo == 3)
            {
                this->demoRequests -= 2;
                this->repulicRequests -= 2;
            }
            else if (selectedCombo = 1)
            {
                this->demoRequests -= 4;
            }
            else
            {
                this->repulicRequests -= 4;
            }
        }

        m_cv.notify_all();
    }

    void RequestRepublic()
    {
        std::unique_lock<std::mutex> l_demoLock(m_rideMutex);
        repulicRequests++;
        m_cv.wait(l_demoLock, [this]() -> bool { return this->demoRequests >= 4 || this->repulicRequests >= 4 || (this->demoRequests >= 2 && this->repulicRequests >= 2); });
        republicSeated++;
        Seated();
        peopleSeated++;

        if (peopleSeated == 4)
        {
            peopleSeated = 0;
            selectedCombo = 0;

            if (selectedCombo == 3)
            {
                this->demoRequests -= 2;
                this->repulicRequests -= 2;
            }
            else if (selectedCombo = 1)
            {
                this->demoRequests -= 4;
            }
            else
            {
                this->repulicRequests -= 4;
            }
        }

        m_cv.notify_all();
    }
};