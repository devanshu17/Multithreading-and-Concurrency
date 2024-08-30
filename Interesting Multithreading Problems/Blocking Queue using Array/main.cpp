#include <vector>
#include <mutex>
#include <condition_variable>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <iostream> 

template<typename T>
class BlockingQueueWithMutex {
public:
    explicit BlockingQueueWithMutex(size_t capacity) : capacity(capacity), size(0), head(0), tail(0) {
        array.resize(capacity);
    }

    void enqueue(const T& item) {
        std::unique_lock<std::mutex> lock(mutex);
        not_full.wait(lock, [this]() { return size < capacity; });

        array[tail] = item;
        tail = (tail + 1) % capacity;
        ++size;

        not_empty.notify_one();
    }

    T dequeue() {
        std::unique_lock<std::mutex> lock(mutex);
        not_empty.wait(lock, [this]() { return size > 0; });

        T item = array[head];
        head = (head + 1) % capacity;
        --size;

        not_full.notify_one();
        return item;
    }

private:
    std::vector<T> array;
    std::mutex mutex;
    std::condition_variable not_full;
    std::condition_variable not_empty;
    size_t size;
    size_t capacity;
    size_t head;
    size_t tail;
};


void producer(BlockingQueueWithMutex<int>& bq, int start, int increment) {
    int i = start;
    while (true) {
        bq.enqueue(i);
        std::cout << "PRODUCING : " << i << std::endl;
        i += increment;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void consumer(BlockingQueueWithMutex<int>& bq) {
    while (true) {
        int item = bq.dequeue();
        std::cout << "CONSUMED: " << item << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

int main() {
    BlockingQueueWithMutex<int> bq(3);
    
    std::thread t1(producer, std::ref(bq), 1, 1);
    std::thread t2(producer, std::ref(bq), 5000, 1);
    std::thread t3(producer, std::ref(bq), 10000, 1);
    
    std::thread t4(consumer, std::ref(bq));
    std::thread t5(consumer, std::ref(bq));
    
    t1.detach();
    t2.detach();
    t3.detach();
    t4.detach();
    t5.detach();
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    return 0;
}