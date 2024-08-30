#include <thread>
#include <condition_variable>

class ReaderWriterLock
{
    int readerCount;
    int writerWaiting;
    bool isWritingInProgress;
    std::condition_variable m_cv;
    std::mutex m_readWrite;

    public:
    
    ReaderWriterLock()
    {
        readerCount = 0;
        writerWaiting = 0;
        isWritingInProgress = false;
    }

    void AcquireReadLock()
    {
        std::unique_lock<std::mutex> l_lock(m_readWrite);
        m_cv.wait(l_lock, [this]() -> bool { return this->writerWaiting == 0 && !this->isWritingInProgress; });
        readerCount++;
    }

    void ReleaseReadLock()
    {
        {
            std::lock_guard<std::mutex> l_lock(m_readWrite);
            readerCount--;
        }

        m_cv.notify_all();
    }

    void AcquireWriteLock()
    {
        std::unique_lock<std::mutex> l_lock(m_readWrite);
        writerWaiting++;
        m_cv.wait(l_lock, [this]() -> bool { return this->readerCount == 0 && this->isWritingInProgress == false; });
        writerWaiting--;
        isWritingInProgress = true;
    }

    void ReleaseWriteLock()
    {
         std::unique_lock<std::mutex> l_lock(m_readWrite);
         isWritingInProgress = false;
         m_cv.notify_all();
    }
};

int main()
{
    return 0;
}