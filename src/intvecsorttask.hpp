#include <random>
#include <vector>

#include "task.hpp"

class IntVecSortTask : public Task {

private:
	std::vector<int> _data;

public:
	IntVecSortTask() {}
	~IntVecSortTask() override = default;

	void randomize(int size) {
		static std::random_device rd;
		static std::mt19937 gen(rd());
		std::uniform_int_distribution<int> dist(0, size*10);
		_data.clear();
		for (int i=0; i<size; i++)
			_data.push_back(dist(gen));
	}

	std::vector<int>& result() {
		return _data;
	}

	// Task interface implementation: split, merge, solve, write

	int split(Task** partitions, int max) override {
		if (_data.size() < 3 || max < 3) return 0;
		IntVecSortTask* leftt = new IntVecSortTask();
		IntVecSortTask* rightt = new IntVecSortTask();
		std::vector<int>& left = leftt->_data;
		std::vector<int>& right = rightt->_data;
		int pivot = _data[0];
		for (int i=1; i<_data.size(); i++) {
			if (_data[i] < pivot)
				left.push_back(_data[i]);
			else
				right.push_back(_data[i]);
		}
		if (right.size())
			left.push_back(pivot);
		else
			right.push_back(pivot);
		partitions[0] = leftt;
		partitions[1] = rightt;
		return 2;
	}

	void merge(Task** p, int max) override {
		_data.clear();
		for (int i=0; i<max; i++) {
			IntVecSortTask *t = (IntVecSortTask *) p[i];
			if (t) {
				p[i] = nullptr;
				for (int v : t->_data)
					_data.push_back(v);
				delete t;
			}
		}
	}

	void solve() override {
		std::sort(_data.begin(), _data.end());
	}

	void write(std::ostream& os) const override {
		for (int i=0; i<_data.size(); i++) {
			if (i) os << ' ';
			os << _data[i];
		}
	}
};
