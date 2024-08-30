#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include <chrono>
#include <random>
#include <functional>

enum class PersonType { MALE, FEMALE };

class Person {
public:
    Person(std::string name, PersonType type) : name(name), type(type) {}
    std::string name;
    PersonType type;
    int getUsageTime() {
        // Simple hash function to generate a usage time based on the name
        int time = 0;
        for (char c : name) {
            time += c;
        }
        return (time % 10) + 1; // Return a value between 1 and 10 seconds
    }
};

class Bathroom {
private:
    int capacity;
    int currentSize;
    PersonType currentType;
    std::mutex mtx;
    std::condition_variable cv;

public:
    Bathroom(int cap) : capacity(cap), currentSize(0), currentType(PersonType::MALE) {}

    int getCapacity() { return capacity; }
    int getCurrentSize() { return currentSize; }
    void incrementOccupancy() { currentSize++; }
    void decrementOccupancy() { currentSize--; }
    PersonType getBathroomType() { return currentType; }
    void setBathroomType(PersonType type) { currentType = type; }

    void use(Person& person) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this, &person] {
            return (currentSize == 0 || (currentSize < capacity && currentType == person.type));
        });

        incrementOccupancy();
        setBathroomType(person.type);

        lock.unlock();

        // Simulate bathroom usage
        std::this_thread::sleep_for(std::chrono::seconds(person.getUsageTime()));

        lock.lock();
        decrementOccupancy();
        if (currentSize == 0) {
            setBathroomType(PersonType::MALE); // Reset to default when empty
        }
        cv.notify_all();
    }
};

class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;

public:
    ThreadPool(size_t threads) : stop(false) {
        for (size_t i = 0; i < threads; ++i)
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                        if (this->stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            });
    }

    template<class F>
    void enqueue(F&& f) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.emplace(std::forward<F>(f));
        }
        condition.notify_one();
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread& worker : workers)
            worker.join();
    }
};

class BathroomManager {
private:
    Bathroom bathroom;
    std::queue<Person> maleQueue;
    std::queue<Person> femaleQueue;
    ThreadPool pool;

public:
    BathroomManager(int capacity) : bathroom(capacity), pool(std::thread::hardware_concurrency()) {}

    void addPerson(Person person) {
        if (person.type == PersonType::MALE) {
            maleQueue.push(person);
        } else {
            femaleQueue.push(person);
        }
        pool.enqueue([this, person] { processPerson(person); });
    }

    void processPerson(Person person) {
        bathroom.use(person);
        std::cout << person.name << " of type " << (person.type == PersonType::MALE ? "MALE" : "FEMALE") 
                  << " used the bathroom for " << person.getUsageTime() << " seconds." << std::endl;
    }
};

int main() {
    BathroomManager manager(3);

    std::vector<Person> people = {
        {"John", PersonType::MALE},
        {"Mary", PersonType::FEMALE},
        {"Bob", PersonType::MALE},
        {"Alice", PersonType::FEMALE},
        {"Charlie", PersonType::MALE},
        {"Eve", PersonType::FEMALE},
    };

    for (const auto& person : people) {
        manager.addPerson(person);
    }

    // Allow some time for all tasks to complete
    std::this_thread::sleep_for(std::chrono::seconds(60));

    return 0;
}