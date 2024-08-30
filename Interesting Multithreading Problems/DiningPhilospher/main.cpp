#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>

using namespace std;

class Philosopher {
private:
    int id;
    mutex* leftFork;
    mutex* rightFork;

public:
    Philosopher(int id, mutex* left, mutex* right) : id(id), leftFork(left), rightFork(right) {}

    void contemplate() {
        cout << "Philosopher " << id << " is contemplating." << endl;
        this_thread::sleep_for(chrono::milliseconds(1000)); // Simulate contemplation
    }

    void eat() {
        // Lock forks in a specified order to avoid deadlock
        unique_lock<mutex> leftLock(*leftFork, defer_lock);
        unique_lock<mutex> rightLock(*rightFork, defer_lock);

        // Lock both forks (lowest first)
        lock(leftLock, rightLock);

        // Now the philosopher is eating
        cout << "Philosopher " << id << " is eating." << endl;
        this_thread::sleep_for(chrono::milliseconds(1000)); // Simulate eating

        // Locks are released automatically when the unique_lock objects go out of scope
    }

    void dine() {
        while (true) {
            contemplate(); // Contemplate for a while
            eat();         // Then try to eat
        }
    }
};

int main() {
    // Create a fork (mutex) for each philosopher
    vector<mutex> forks(5);

    // Create philosophers and assign their respective forks
    vector<thread> philosophers;
    for (int i = 0; i < 5; ++i) {
        philosophers.push_back(thread(&Philosopher::dine, Philosopher(i, &forks[i], &forks[(i + 1) % 5])));
    }

    // Join all philosopher threads to the main thread
    for (auto& philosopher : philosophers) {
        philosopher.join();
    }

    return 0;
}
