#include <iostream>
#include <thread>
#include <vector>
#include <mutex>

using namespace std;

class CountPrime {
public:
    CountPrime(int maxNum, int numThreads) 
        : maxNum(maxNum), numThreads(numThreads), currentNum(1), totalPrime(0) {}

    int getTotalPrimeCount() {
        // Launch threads
        for (int i = 0; i < numThreads; i++) {
            threads.emplace_back(&CountPrime::checkPrimes, this);
        }
        
        // Wait for all threads to finish
        for (auto &t : threads) {
            t.join();
        }
        
        return totalPrime;
    }

private:
    int maxNum;
    int numThreads;
    int totalPrime;
    int currentNum;
    vector<thread> threads;
    mutex mtxCurrentNum; // Mutex for synchronizing access to currentNum
    mutex mtxTotalPrime; // Mutex for synchronizing access to totalPrime

    bool isPrime(int x) {
        if (x <= 1) return false;
        if (x <= 3) return true;
        if (x % 2 == 0 || x % 3 == 0) return false;
        for (int i = 5; i * i <= x; i += 6) {
            if (x % i == 0 || x % (i + 2) == 0) return false;
        }
        return true;
    }

    void checkPrimes() {
        while (true) {
            int num;
            
            // Lock the mutex for currentNum to get and increment the current number
            {
                lock_guard<mutex> lock(mtxCurrentNum);
                if (currentNum > maxNum) break;
                num = currentNum++;
            }
            
            // Check if the number is prime
            if (isPrime(num)) {
                // Lock the mutex for totalPrime to update the prime count
                lock_guard<mutex> lock(mtxTotalPrime);
                totalPrime++;
            }
        }
    }
};

int main() {
    cout << "Starting prime count..." << endl;
    
    CountPrime countPrime(100000000, 10);
    cout << "Total prime numbers up to 1000000: " << countPrime.getTotalPrimeCount() << endl;

    return 0;
}
