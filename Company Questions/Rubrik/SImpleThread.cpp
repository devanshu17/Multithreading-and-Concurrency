#include <thread>
#include <iostream>

using namespace std;

void Test(int x)
{
    while (1)
    {
        cout<<" here in child thread "<<x<<endl;
    }
}

int main()
{
    std::thread child(&Test, 19);
    child.detach();

    cout<<"here in main thread"<<endl;
}