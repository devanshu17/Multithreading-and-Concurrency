#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <string>
#include <chrono>
#include <functional>

using namespace std;

class BathroomManager {
public:
    BathroomManager() 
        : menInside(0), womenInside(0), currentTurn(GENDER_NONE), consecutiveMenTurns(0), consecutiveWomenTurns(0), maxConsecutiveTurns(3) {}

    void useBathroom(string name, char gender) {
        unique_lock<mutex> lock(mtx);
        if (gender == 'M') {
            menQueue.push(name);
        } else if (gender == 'W') {
            womenQueue.push(name);
        }

        cv.wait(lock, [this, gender]() {
            if (gender == 'M') {
                return (currentTurn == GENDER_NONE || currentTurn == GENDER_MEN) && 
                        menInside < 3 && womenInside == 0 && 
                        (consecutiveMenTurns < maxConsecutiveTurns || womenQueue.empty());
            } else {
                return (currentTurn == GENDER_NONE || currentTurn == GENDER_WOMEN) && 
                        womenInside < 3 && menInside == 0 && 
                        (consecutiveWomenTurns < maxConsecutiveTurns || menQueue.empty());
            }
        });

        if (gender == 'M') {
            menInside++;
            menQueue.pop();
            currentTurn = GENDER_MEN;
            consecutiveMenTurns++;
            consecutiveWomenTurns = 0;  // Reset women counter
        } else {
            womenInside++;
            womenQueue.pop();
            currentTurn = GENDER_WOMEN;
            consecutiveWomenTurns++;
            consecutiveMenTurns = 0;  // Reset men counter
        }
        
        lock.unlock();
        
        // Simulate bathroom usage
        int duration = f(name);
        cout << name << " (" << gender << ") is using the bathroom for " << duration << " seconds." << endl;
        this_thread::sleep_for(chrono::seconds(duration));

        lock.lock();
        if (gender == 'M') {
            menInside--;
            if (menInside == 0) {
                currentTurn = GENDER_NONE;
                cv.notify_all();  // Notify waiting women
            }
        } else {
            womenInside--;
            if (womenInside == 0) {
                currentTurn = GENDER_NONE;
                cv.notify_all();  // Notify waiting men
            }
        }
    }

private:
    enum Gender { GENDER_NONE, GENDER_MEN, GENDER_WOMEN };
    Gender currentTurn;

    mutex mtx;
    condition_variable cv;
    queue<string> menQueue;
    queue<string> womenQueue;
    int menInside;
    int womenInside;
    
    int consecutiveMenTurns;
    int consecutiveWomenTurns;
    const int maxConsecutiveTurns;  // Maximum allowed consecutive turns for one gender

    int f(const string& name) {
        // Dummy function for demonstration. Replace with the actual logic.
        return 1 + (name.length() % 5);
    }
};

// Worker function for threads
void person(BathroomManager& manager, string name, char gender) {
    manager.useBathroom(name, gender);
}

int main() {
    BathroomManager manager;

    thread t1(person, ref(manager), "Alice", 'W');
    thread t2(person, ref(manager), "Bob", 'M');
    thread t3(person, ref(manager), "Charlie", 'M');
    thread t4(person, ref(manager), "Dave", 'M');
    thread t5(person, ref(manager), "Eve", 'W');
    thread t6(person, ref(manager), "Frank", 'M');
    thread t7(person, ref(manager), "Grace", 'W');

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    t6.join();
    t7.join();

    return 0;
}
