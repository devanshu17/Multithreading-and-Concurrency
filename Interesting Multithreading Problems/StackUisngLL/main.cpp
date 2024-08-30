#include <thread>
#include <condition_variable>
#include <mutex>
#include <functional>
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>

using namespace std;

class LinkedList{
    private:
    struct Node{
        int val;
        Node* next;
        Node(int val,Node* next) : val(val), next(next) {}
        Node(int val) : val(val), next(nullptr) {}
        
        Node() : next(nullptr) {}
    };
    
    Node* dummyHead;
    
    int size;
    
    public:
        LinkedList() : size(0){
            dummyHead = new Node();
        }
        
        void Insert(int val,int pos)
        {
            if(size < pos || pos==0)
                return;
            
            Node* cur = dummyHead;
            
            while(pos--)
            {
                cur = cur->next;
            }
            
            auto temp = cur->next;
            auto newNode = new Node(val,temp);
            cur->next = newNode;
            size++;
        }
        
        int Delete(int pos)
        {
            if(size < pos || pos==0)
            {
                return -1;
            }
            
            Node* cur = dummyHead;
            while(pos--)
                cur = cur->next;
                
            auto temp = cur->next;
            cur->next = cur->next->next;
            int val = temp->val;
            delete temp;
            size--;
            
            return val;
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
 
 class StackUsingLL{
        private:
            LinkedList ll;
            mutex mtx;
            
        public:
            void Push(int val)
            {
                lock_guard<mutex> lock(mtx);
                cout << " PUSHING: " << val << endl;
                ll.Insert(val,1);
            }
            
            int Pop()
            {
                lock_guard<mutex> lock(mtx);
                int val = ll.Delete(1);
                cout << " Poped " << val << endl;
                return val;
            }
            
            int GetSize()
            {
                lock_guard<mutex> lock(mtx);
                return ll.GetSize();
            }
 };

int main()
{
    StackUsingLL stack;
    
    vector<function<void()>> functions = {
        [&](){ stack.Push(1); },
        [&](){ stack.Pop(); }
    };
    
    default_random_engine engine(random_device{}());
    
    vector<thread> threads;
    
    for(int i=0;i<50;i++)
    {
        shuffle(functions.begin(), functions.end(), engine);
        for(auto &f:functions)
            threads.push_back(thread(f));
    }
    
    for(auto& thread: threads)
        thread.join();

    return 0;
}