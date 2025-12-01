#include <iostream>
#include <chrono>
#include <algorithm>

class Task
{
public:
	virtual int split(Task **vector, int max) = 0;
	virtual void merge(Task **vector, int max) = 0;
	virtual void solve() = 0;
	virtual void write(std::ostream &os) const = 0;
	virtual ~Task() = default;
};

class TaskRunner
{
private:
	std::chrono::time_point<std::chrono::high_resolution_clock> _start, _stop;

public:
	virtual void run(Task *t) = 0;
	virtual ~TaskRunner() = default;

	double duration() const
	{
		std::chrono::duration<double> diff = _stop - _start;
		return diff.count(); // seconds as a double
	}

protected:
	void startTimer() { _start = std::chrono::high_resolution_clock::now(); }
	void stopTimer() { _stop = std::chrono::high_resolution_clock::now(); }
};

class DirectTaskRunner : public TaskRunner
{
public:
	virtual void run(Task *t) override
	{
		TaskRunner::startTimer();
		t->solve();
		TaskRunner::stopTimer();
	}
};

class PartitionedTaskRunner : public TaskRunner
{
private:
	int _max;
	void recurse(Task *t)
	{
		Task *partitions[_max];
		for (int i = 0; i < _max; i++)
			partitions[i] = 0;
		int npart = t->split(partitions, _max);
		if (npart)
		{
			for (int i = 0; i < npart; i++)
				recurse(partitions[i]);
			t->merge(partitions, npart);
		}
		else
			t->solve();
	}

	PartitionedTaskRunner() {} // cannot use default constructor

public:
	PartitionedTaskRunner(int max) : _max(max) {}

	virtual void run(Task *t) override
	{
		TaskRunner::startTimer();
		recurse(t);
		TaskRunner::stopTimer();
	}
};

std::ostream &operator<<(std::ostream &os, const Task &t)
{
	t.write(os);
	return os;
}
