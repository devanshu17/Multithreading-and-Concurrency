#include <iostream>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <vector>

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
    mutable std::shared_mutex cacheMutex; // Shared mutex for thread-safety

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
        std::shared_lock<std::shared_mutex> readLock(cacheMutex); // Shared lock for concurrent reads

        if (m.find(key) != m.end()) {
            Node* resNode = m[key];
            int ans = resNode->val;

            readLock.unlock(); // Unlock the read lock before locking for write
            std::unique_lock<std::shared_mutex> writeLock(cacheMutex); // Unique lock for exclusive write

            m.erase(key);
            deleteNode(resNode);
            addNode(resNode);
            m[key] = head->next;

            return ans;
        }
        return -1;
    }
    
    void put(int key, int value) {
        std::unique_lock<std::shared_mutex> writeLock(cacheMutex); // Unique lock for exclusive write

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

void worker(LRUCache& cache, int thread_id) {
    // Each thread will perform a combination of get and put operations
    for (int i = 0; i < 10; ++i) {
        int key = i + thread_id * 10;
        int value = key * 10;

        cache.put(key, value); // Put operation
        std::cout << "Thread " << thread_id << " put key=" << key << " value=" << value << std::endl;

        int result = cache.get(key); // Get operation
        std::cout << "Thread " << thread_id << " got key=" << key << " value=" << result << std::endl;
    }
}

int main() {
    LRUCache cache(10); // LRU cache with capacity 10

    std::vector<std::thread> threads;

    // Create multiple threads to work on the LRU cache concurrently
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(worker, std::ref(cache), i);
    }

    // Join all threads
    for (auto& thread : threads) {
        thread.join();
    }

    return 0;
}
