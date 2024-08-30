#include <thread>
#include <condition_variable>
#include <mutex>
#include <functional>
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
using namespace std;


class LinkedList
{
    struct Node
    {
        int val;
        Node* next;

        Node()
        {
            next = nullptr;
        }

        Node(int p_val)
        {
            val = p_val;
            next = nullptr;
        }

        Node(int p_val, Node* p_next)
        {
            val = p_val;
            next = p_next;
        }
    };

    Node* dummyHead;
    int size;

    public:

    LinkedList()
    {
        dummyHead = new Node();
        size = 0;
    }

    void Insert(int val, int pos)
    {
        if (size < pos || pos == 0)
        {
            return;
        }

        Node* walker = dummyHead;

        while (pos--)
        {
            walker = walker->next;
        }

        auto temp = walker->next;
        auto newNode = new Node(val, temp);
        walker->next = newNode;
        size++;
    }

    int Delete(int pos)
    {
        if (size < pos || pos == 0)
        {
            return -1;
        }

        Node* walker = dummyHead;

        while (pos--)
        {
            walker = walker->next;
        }

        auto temp = walker->next;
        walker->next = walker->next->next;
        int value = temp->val;
        delete temp;
        size--;

        return value;
    }

    Node* GetHead()
    {
        return dummyHead->next;
    }

    int GetSize()
    {
        return size;
    }
};

class StackUsingLL
{
    LinkedList ll;
    mutex m;

    public:

    void Push(int val)
    {
        std::lock_guard<mutex> l_lock(m);
        cout<<" pushing "<<endl;
        ll.Insert(val, 1);
    }

    int Pop()
    {
        std::lock_guard<mutex> l_lock(m);
        cout<<" popping "<<endl;
        return ll.Delete(1);
    }

    int GetSize()
    {
        std::lock_guard<mutex> l_lock(m);
        return ll.GetSize();
    }
};

int main()
{
    // Create a StackUsingLL object
    StackUsingLL stack;

    // Create a vector of function pointers to the StackUsingLL methods
    std::vector<std::function<void()>> functions = {
        [&]() { stack.Push(1); },
        [&]() { 
                stack.Pop(); 
        }
    };

    // Create a random engine
    std::default_random_engine engine(std::random_device{}());

    // Create a vector of threads
    std::vector<std::thread> threads;

    // Make 1M calls to each function in random order
    for (int i = 0; i < 1000; ++i) {
        // Shuffle the functions
        std::shuffle(functions.begin(), functions.end(), engine);

        // Call each function in the shuffled order
        for (auto& function : functions) {
            threads.push_back(std::thread(function));
        }
    }

    // Join all threads
    for (auto& thread : threads) {
        thread.join();
    }

    return 0;
}