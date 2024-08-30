#include <thread>
#include <condition_variable>
#include <iostream>
#include <vector>
#include <functional>

using namespace std;

class UberRide {
private:
	mutex mtx;
	condition_variable cv;
	int democratesRequests;
	int republicansRequests;
	int peopleSeated;
	int selectedCombo;

	void seated()
	{
		cout << " Seated " << this_thread::get_id() << endl;
	}

	void drive()
	{
		cout << "Car driven with configuration: ";
		if(selectedCombo==1)
		{
			cout << " 4 Democrates" << endl;
		}
		else if(selectedCombo==2)
		{
			cout << " 4 Republicans" << endl;
		}
		if(selectedCombo==3)
		{
			cout << " 2 Democrates, 2 Republicans" << endl;
		}
	}

public:
	UberRide() : democratesRequests(0), republicansRequests(0), peopleSeated(0), selectedCombo(0) {}

	void requestRepublican()
	{
		unique_lock<mutex> lock(mtx);
		republicansRequests++;
		cv.wait_for(lock,10s,[this] { return democratesRequests>=4 || republicansRequests>=4 || (democratesRequests>=2 && republicansRequests>=2);});

		
		seated();
		peopleSeated++;

		if(peopleSeated==4)
		{
			peopleSeated=0;
			if(democratesRequests>=4)
			{
				democratesRequests-=4;
				selectedCombo=1;
			}
			else if(republicansRequests>=4)
			{
				republicansRequests -= 4;
				selectedCombo=2;
			}
			else if(democratesRequests>=2 && republicansRequests>=2)
			{
				democratesRequests-=2;
				republicansRequests-=2;
				selectedCombo=3;
			}
			drive();
		}
		cv.notify_all();
	}

	void requestDemocrates()
	{
		unique_lock<mutex> lock(mtx);
		democratesRequests++;
		cv.wait_for(lock,10s,[this] { return democratesRequests>=4 || republicansRequests>=4 || (democratesRequests>=2 && republicansRequests>=2);});

		seated();
		peopleSeated++;

		if(peopleSeated==4)
		{
			peopleSeated=0;
			if(democratesRequests>=4)
			{
				democratesRequests-=4;
				selectedCombo=1;
			}
			else if(republicansRequests>=4)
			{
				republicansRequests -= 4;
				selectedCombo=2;
			}
			else if(democratesRequests>=2 && republicansRequests>=2)
			{
				democratesRequests-=2;
				republicansRequests-=2;
				selectedCombo=3;
			}
			drive();
		}
		cv.notify_all();
	}

};

void democrates(UberRide &ride)
{
	ride.requestDemocrates();
}

void republicans(UberRide &ride)
{
	ride.requestRepublican();
}


int main()
{
	UberRide ride;

    thread t1(republicans, ref(ride));
    thread t2(republicans, ref(ride));
    // thread t3(democrates, ref(ride));
    // thread t4(democrates, ref(ride));

    thread t5(republicans, ref(ride));
    thread t6(republicans, ref(ride));
    thread t7(democrates, ref(ride));
    thread t8(democrates, ref(ride));

    t1.join();
    t2.join();
    // t3.join();
    // t4.join();
    t5.join();
    t6.join();
    t7.join();
    t8.join();

	return 0;
}