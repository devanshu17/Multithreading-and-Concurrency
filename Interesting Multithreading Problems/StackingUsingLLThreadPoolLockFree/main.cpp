#include <thread>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <vector>
#include <queue>
#include <chrono>

using namespace std;

class LinkedList {
private:
    struct Node {
        int val;
        Node* next;
        Node(int val, Node* next) : val(val), next(next) {}
        Node(int val) : val(val), next(nullptr) {}

        Node() : next(nullptr) {}
    };

    Node* dummyHead;
    int size;

public:
    LinkedList() : size(0) {
        dummyHead = new Node();
    }

    void Insert(int val, int pos) {
        if (pos > size + 1 || pos <= 0)  // Fix: Insert position is 1-based and valid only if 1 <= pos <= size + 1
            return;

        Node* cur = dummyHead;

        while (--pos) {  // Fix: Adjust the position decrement
            cur = cur->next;
        }

        auto temp = cur->next;
        auto newNode = new Node(val, temp);
        cur->next = newNode;
        size++;
    }

    int Delete(int pos) {
        if (pos > size || pos <= 0)  // Fix: Ensure the position is within bounds (1-based)
            return -1;

        Node* cur = dummyHead;
        while (--pos)  // Fix: Adjust the position decrement
            cur = cur->next;

        auto temp = cur->next;
        cur->next = cur->next->next;
        int val = temp->val;
        delete temp;
        size--;

        return val;
    }

    Node* GetHead() {
        return dummyHead->next;
    }

    int GetSize() {
        return size;
    }
};

class StackUsingLL {
private:
    LinkedList ll;

public:
    void Push(int val) {
        cout << " PUSHING: " << val << endl;
        ll.Insert(val, 1);
    }

    int Pop() {
        if (ll.GetSize() == 0) {  // Fix: Check for empty stack before popping
            cout << " Stack is empty, cannot pop" << endl;
            return -1;
        }
        int val = ll.Delete(1);
        cout << " Popped " << val << endl;
        return val;
    }

    int GetSize() {
        return ll.GetSize();
    }
};

class ThreadPool {
private:
    queue<function<void()>> taskQueue;
    mutex mtx;
    condition_variable cv;
    vector<thread> workers;
    bool stop;

    void run() {
        while (true) {
            unique_lock<mutex> lock(mtx);
            cv.wait(lock, [this] { return stop || !taskQueue.empty(); });

            if (stop && taskQueue.empty())
                return;

            auto task = taskQueue.front();
            taskQueue.pop();
            lock.unlock();

            task();
        }
    }

public:
    ThreadPool(int numThreads) : stop(false) {
        for (int i = 0; i < numThreads; i++) {
            workers.emplace_back(&ThreadPool::run, this);
        }
    }

    void scheduleTask(const function<void()>& task) {
        unique_lock<mutex> lock(mtx);
        taskQueue.emplace(task);
        lock.unlock();
        cv.notify_one();
    }

    ~ThreadPool() {
        unique_lock<mutex> lock(mtx);
        stop = true;
        lock.unlock();
        cv.notify_all();
        for (auto& t : workers)
            t.join();
    }
};

void workerFunction(ThreadPool& pool, StackUsingLL& stack, int tId) {
    for (int i = 0; i < 10; i++) {
        int val = i + tId * 10;
        pool.scheduleTask([&stack, tId, val]() {
            stack.Push(val);
            cout << "thread " << tId << " put val " << val << endl;
        });

        this_thread::sleep_for(chrono::milliseconds(100));

        pool.scheduleTask([&stack, tId]() {
            int popVal = stack.Pop();
            cout << "thread " << tId << " popped val " << popVal << endl;
        });

        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

int main() {
    StackUsingLL stack;
    ThreadPool pool(4);

    vector<thread> threads;

    for (int i = 0; i < 8; i++)
        threads.emplace_back(workerFunction, ref(pool), ref(stack), i);

    for (auto& thread : threads)
        thread.join();

    return 0;
}
