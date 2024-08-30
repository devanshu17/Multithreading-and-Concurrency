#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <functional>
#include <atomic>

using namespace std;

enum class PersonType { MALE, FEMALE };

class Person {
public:
    Person(string name, PersonType type) : name(name), type(type) {}
    string getName() const { return name; }
    PersonType getType() const { return type; }
    int getUsageTime() const { return f(name); }

private:
    string name;
    PersonType type;

    int f(const string& name) const {
        // Simple hash function to generate varying usage times
        int sum = 0;
        for (char c : name) {
            sum += c;
        }
        return (sum % 10 + 1) * 1000; // 1-10 seconds
    }
};

class Bathroom {
public:
    Bathroom() : capacity(3), currentSize(0), currentType(nullptr) {}

    int getCapacity() const { return capacity; }
    int getCurrentSize() const { return currentSize; }
    void incrementOccupancy() { ++currentSize; }
    void decrementOccupancy() { --currentSize; }
    PersonType* getBathroomType() const { return currentType; }
    void setBathroomType(PersonType* type) { currentType = type; }

private:
    const int capacity;
    atomic<int> currentSize;
    PersonType* currentType;
};

class BlockingQueue {
public:
    void push(Person person) {
        unique_lock<mutex> lock(mtx);
        bq.push(person);
        cv.notify_one();
    }

    Person pop() {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this] { return !bq.empty(); });
        Person person = bq.front();
        bq.pop();
        return person;
    }

    bool empty() const {
        lock_guard<mutex> lock(mtx);
        return bq.empty();
    }

private:
    queue<Person> bq;
    mutable mutex mtx;
    condition_variable cv;
};

class BathroomManager {
public:
    BathroomManager() : bathroom(), scheduler_thread(&BathroomManager::scheduler, this) {}

    void addPerson(const Person& person) {
        if (person.getType() == PersonType::MALE) {
            male_queue.push(person);
        } else {
            female_queue.push(person);
        }
    }

    void run() {
        scheduler_thread.join();
    }

private:
    Bathroom bathroom;
    BlockingQueue male_queue;
    BlockingQueue female_queue;
    thread scheduler_thread;
    mutex bathroom_mutex;
    condition_variable bathroom_cv;
    bool is_male_turn = true;

    // scheduling policy used is a combination of First-In-First-Out (FIFO) with priority alternation between male and female queues.
    // Within the male_queue and female_queue, people are processed in the order they arrive
    // 
    void scheduler() {
        while (true) {
            unique_lock<mutex> lock(bathroom_mutex);

            if (bathroom.getCurrentSize() == 0) {
                if (is_male_turn && !male_queue.empty()) {
                    processQueue(male_queue, PersonType::MALE);
                } else if (!is_male_turn && !female_queue.empty()) {
                    processQueue(female_queue, PersonType::FEMALE);
                } else if (!female_queue.empty()){
                    processQueue(female_queue, PersonType::FEMALE);
                } 
                else if (!male_queue.empty()){
                    processQueue(male_queue, PersonType::MALE);
                }
                else {
                    // allows the scheduler thread to wait efficiently when there's no work to be done, without consuming CPU cycles.
                    //  Releases the bathroom_mutex lock.Puts the current thread (scheduler) to sleep.
                    bathroom_cv.wait(lock);
                    continue;
                }
                // Toggle the turn after processing a queue when bathroom was empty
                is_male_turn = !is_male_turn;
            } else {
                PersonType current_type = *bathroom.getBathroomType();
                if (current_type == PersonType::MALE && !male_queue.empty()) {
                    processQueue(male_queue, PersonType::MALE);
                } else if (current_type == PersonType::FEMALE && !female_queue.empty()) {
                    processQueue(female_queue, PersonType::FEMALE);
                } else {
                    bathroom_cv.wait(lock);
                }
            }
        }
    }

    void processQueue(BlockingQueue& queue, PersonType type) {
        while (bathroom.getCurrentSize() < bathroom.getCapacity() && !queue.empty()) {
            Person person = queue.pop();
            bathroom.incrementOccupancy();
            if (bathroom.getCurrentSize() == 1) {
                bathroom.setBathroomType(&type);
            }

            thread([this, person]() {
                cout << person.getName() << " (" << (person.getType() == PersonType::MALE ? "M" : "F")
                     << ") entered the bathroom." << endl;
                
                this_thread::sleep_for(chrono::milliseconds(person.getUsageTime()));
                
                unique_lock<mutex> lock(bathroom_mutex);
                bathroom.decrementOccupancy();
                cout << person.getName() << " (" << (person.getType() == PersonType::MALE ? "M" : "F")
                     << ") left the bathroom." << endl;

                if (bathroom.getCurrentSize() == 0) {
                    bathroom.setBathroomType(nullptr); // By allowing the bathroom type to reset to null when it becomes 
                                                      // empty, and then choosing the non-empty queue to service next, 
                                                      // the system prevents deadlocks where one queue might be waiting indefinitely.
                }
                bathroom_cv.notify_all();
            }).detach();
        }
    }
};

int main() {
    BathroomManager manager;

    // Add some people to the queues
    manager.addPerson(Person("Alice", PersonType::FEMALE));
    manager.addPerson(Person("Bob", PersonType::MALE));
    manager.addPerson(Person("Charlie", PersonType::MALE));
    manager.addPerson(Person("Diana", PersonType::FEMALE));
    manager.addPerson(Person("Eve", PersonType::FEMALE));
    manager.addPerson(Person("Frank", PersonType::MALE));

    manager.run();

    return 0;
}