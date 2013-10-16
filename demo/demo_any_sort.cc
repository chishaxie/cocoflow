#include <iostream>

#include "cocoflow.h"

using namespace std;

#define WAITING_TIME 1000

class sort_task: public ccf::user_task
{
protected:
	int vals[10];
	int num;
	bool right;
	void show_cur_vals(int compare_a = -1, int compare_b = -1, int _small = -1, int _large = -1)
	{
#if defined(_WIN32) || defined(_WIN64)
		compare_a = compare_b = _small = _large = -1;
#endif
		ccf::sleep waiting(2 * WAITING_TIME);
		await(waiting);
		if (this->right) cout << "                              ";
		for (int i=0; i<this->num; i++)
		{
			if (_small == i) cout << "\033[1;31m";
			else if (_large == i) cout << "\033[1;34m";
			else if (compare_a == i || compare_b == i) cout << "\033[1;32m";
			cout << vals[i] << " ";
			if (compare_a == i || compare_b == i || _small == i || _large == i) cout << "\033[0m";
		}
		cout << endl;
	}
	void init(bool right)
	{
		this->right = right;
		this->num = 10;
		vals[0] = 9; vals[1] = 5; vals[2] = 11; vals[3] = 7; vals[4] = 2;
		vals[5] = 21; vals[6] = 8; vals[7] = 4; vals[8] = 15; vals[9] = 9;
	}
};

class bubble_sort_task: public sort_task
{
	void run()
	{
		this->init(false);
		this->show_cur_vals();
		for (int i=0; i<this->num-1; i++)
		{
			bool change = false;
			for (int j=this->num-1; j>=i; j--)
			{
				this->show_cur_vals(j, j-1);
				if (vals[j] < vals[j-1])
				{
					int tmp = vals[j];
					vals[j] = vals[j-1];
					vals[j-1] = tmp;
					change = true;
					this->show_cur_vals(-1, -1, j-1, j);
				}
			}
			if (!change)
				break;
		}
	}
	void cancel()
	{
		cout << "BubbleSort is canceled" << endl;
	}
};

class select_sort_task: public sort_task
{
	void run()
	{
		ccf::sleep waiting(WAITING_TIME);
		await(waiting);
		this->init(true);
		this->show_cur_vals();
		for (int i=0; i<this->num-1; i++)
		{
			int min = i;
			for (int j=i+1; j<this->num; j++)
			{
				this->show_cur_vals(i, j, min);
				if (vals[min] > vals[j])
				{
					min = j;
					this->show_cur_vals(i, j, min);
				}
			}
			if (min != i)
			{
				int tmp = vals[i];
				vals[i] = vals[min];
				vals[min] = tmp;
				this->show_cur_vals(-1, -1, i, min);
			}
		}
	}
	void cancel()
	{
		cout << "SelectSort is canceled" << endl;
	}
};

class main_task: public ccf::user_task
{
	void run()
	{
		cout << "BubbleSort (left)             SelectSort (right)" << endl;
		bubble_sort_task b;
		select_sort_task s;
		ccf::any_of any(b, s);
		await(any);
	}
};

int main()
{
	ccf::event_task::init(100);
	ccf::user_task::init(100);
	
	//ccf::set_debug(stderr);
	
	main_task tMain;
	ccf::cocoflow(tMain);
	return 0;
}
