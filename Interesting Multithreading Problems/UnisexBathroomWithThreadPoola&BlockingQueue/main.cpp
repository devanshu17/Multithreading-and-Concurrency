#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <chrono>

using namespace std;

enum class Gender { NONE, MALE, FEMALE };

class Bathroom {
    int capacity;
    int occupancy;
    Gender currentType;
    mutex mtx;

public:
    Bathroom(int cap) : capacity(cap), occupancy(0), currentType(Gender::NONE) {}

    int getCapacity() {
        return capacity;
    }

    int getCurrentSize() {
        lock_guard<mutex> lock(mtx);
        return occupancy;
    }

    Gender getBathroomType() {
        lock_guard<mutex> lock(mtx);
        return currentType;
    }

    void incrementOccupancy(Gender type) {
        lock_guard<mutex> lock(mtx);
        if (occupancy == 0) {
            currentType = type;
        }
        occupancy++;
    }

    void decrementOccupancy() {
        lock_guard<mutex> lock(mtx);
        occupancy--;
        if (occupancy == 0) {
            currentType = Gender::NONE;
        }
    }

    void setBathroomType(Gender type) {
        lock_guard<mutex> lock(mtx);
        currentType = type;
    }
};

class BlockingQueue {
    queue<function<void()>> q;
    mutex mtx;
    condition_variable cv;

public:
    void push(function<void()> task) {
        unique_lock<mutex> lock(mtx);
        q.push(task);
        cv.notify_one();
    }

    function<void()> pop() {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this] { return !q.empty(); });
        auto task = q.front();
        q.pop();
        return task;
    }

    bool isEmpty() {
        lock_guard<mutex> lock(mtx);
        return q.empty();
    }
};

class ThreadPoolExecutor {
    vector<thread> workers;
    BlockingQueue taskQueue;
    bool stop;

public:
    ThreadPoolExecutor(int numThreads) : stop(false) {
        for (int i = 0; i < numThreads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    auto task = taskQueue.pop();
                    if (stop) break;
                    task();
                }
            });
        }
    }

    void execute(function<void()> task) {
        taskQueue.push(task);
    }

    void shutdown() {
        stop = true;
        for (auto& worker : workers) {
            worker.join();
        }
    }
};

class BathroomManager {
    Bathroom bathroom;
    BlockingQueue menQueue;
    BlockingQueue womenQueue;
    mutex mtx;
    condition_variable cv;
    ThreadPoolExecutor executor;

    int f(string name) {
        // Example function to determine the time a person spends in the bathroom
        return 1000 + (hash<string>{}(name) % 1000); // Sleep time between 1000ms to 1999ms
    }

public:
    BathroomManager(int cap, int numThreads) : bathroom(cap), executor(numThreads) {}

    void addPerson(string name, Gender type) {
        auto task = [this, name, type] {
            unique_lock<mutex> lock(mtx);
            cv.wait(lock, [this, type] {
                return (bathroom.getBathroomType() == Gender::NONE || bathroom.getBathroomType() == type)
                    && bathroom.getCurrentSize() < bathroom.getCapacity();
            });

            bathroom.incrementOccupancy(type);
            lock.unlock();

            cout << name << " (" << (type == Gender::MALE ? "Male" : "Female") << ") entered the bathroom." << endl;
            this_thread::sleep_for(chrono::milliseconds(f(name)));
            cout << name << " (" << (type == Gender::MALE ? "Male" : "Female") << ") left the bathroom." << endl;

            lock.lock();
            bathroom.decrementOccupancy();
            cv.notify_all();
        };

        if (type == Gender::MALE) {
            menQueue.push(task);
        } else {
            womenQueue.push(task);
        }

        executor.execute([this, type] {
            if (type == Gender::MALE) {
                auto task = menQueue.pop();
                task();
            } else {
                auto task = womenQueue.pop();
                task();
            }
        });
    }

    void shutdown() {
        executor.shutdown();
    }
};

int main() {
    BathroomManager manager(3, 4);

    manager.addPerson("John", Gender::MALE);
    manager.addPerson("Alice", Gender::FEMALE);
    manager.addPerson("Bob", Gender::MALE);
    manager.addPerson("Eve", Gender::FEMALE);
    manager.addPerson("Charlie", Gender::MALE);
    manager.addPerson("Diana", Gender::FEMALE);

    this_thread::sleep_for(chrono::seconds(10));  // Wait for some time to allow tasks to complete
    manager.shutdown();

    return 0;
}
