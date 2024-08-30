#include <iostream>
#include <unordered_map>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <functional>
#include <memory>

class LRUCache {
public:
    class Node {
    public:
        int key;
        int val;
        Node* prev;
        Node* next;

        Node(int key, int val) : key(key), val(val), prev(nullptr), next(nullptr) {}
    };

    Node* head = new Node(-1, -1);
    Node* tail = new Node(-1, -1);
    int cap;
    std::unordered_map<int, Node*> m;

    LRUCache(int capacity) : cap(capacity) {
        head->next = tail;
        tail->prev = head;
    }

    void addNode(Node* newnode) {
        Node* temp = head->next;
        newnode->next = temp;
        newnode->prev = head;
        head->next = newnode;
        temp->prev = newnode;
    }

    void deleteNode(Node* delnode) {
        Node* prevv = delnode->prev;
        Node* nextt = delnode->next;
        prevv->next = nextt;
        nextt->prev = prevv;
    }
    
    int get(int key) {
        if (m.find(key) != m.end()) {
            Node* resNode = m[key];
            int ans = resNode->val;

            m.erase(key);
            deleteNode(resNode);
            addNode(resNode);
            m[key] = head->next;

            return ans;
        }
        return -1;
    }
    
    void put(int key, int value) {
        if (m.find(key) != m.end()) {
            Node* curr = m[key];
            m.erase(key);
            deleteNode(curr);
        }

        if (m.size() == cap) {
            m.erase(tail->prev->key);
            deleteNode(tail->prev);
        }

        addNode(new Node(key, value));
        m[key] = head->next;
    }
};

class Scheduler {
private:
    std::queue<std::function<void()>> taskQueue;
    std::mutex mtx;
    std::condition_variable cv;
    bool stop;
    std::thread worker;

    void run() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [this]() { return !taskQueue.empty() || stop; });

                if (stop && taskQueue.empty())
                    return;

                task = std::move(taskQueue.front());
                taskQueue.pop();
            }

            task();
        }
    }

public:
    Scheduler() : stop(false) {
        worker = std::thread(&Scheduler::run, this);
    }

    void scheduleTask(const std::function<void()>& task) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            taskQueue.push(task);
        }
        cv.notify_one();
    }

    void stopScheduler() {
        {
            std::unique_lock<std::mutex> lock(mtx);
            stop = true;
        }
        cv.notify_all();
        if (worker.joinable()) {
            worker.join();
        }
    }

    ~Scheduler() {
        stopScheduler();
    }
};

void workerFunction(Scheduler& scheduler, LRUCache& cache, int thread_id) {
    for (int i = 0; i < 10; ++i) {
        int key = i + thread_id * 10;
        int value = key * 10;

        scheduler.scheduleTask([&cache, key, value, thread_id]() {
            cache.put(key, value);
            std::cout << "Thread " << thread_id << " put key=" << key << " value=" << value << std::endl;
        });

        scheduler.scheduleTask([&cache, key, thread_id]() {
            int result = cache.get(key);
            std::cout << "Thread " << thread_id << " got key=" << key << " value=" << result << std::endl;
        });
    }
}

int main() {
    LRUCache cache(10);
    Scheduler scheduler;

    std::vector<std::thread> threads;

    // Create multiple threads to work on the LRU cache concurrently via the scheduler
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(workerFunction, std::ref(scheduler), std::ref(cache), i);
    }

    // Join all threads
    for (auto& thread : threads) {
        thread.join();
    }

    return 0;
}
