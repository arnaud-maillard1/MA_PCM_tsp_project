#include <bitset>
#include <climits>
#include <atomic>

#include "tspgraph.hpp"
#include "task.hpp"

class TSPPath
{
public:
	static const int FIRST_NODE = 0;
	static const int MAX_GRAPH = 32;

private:
	static TSPGraph *_graph;
	int _node[MAX_GRAPH];
	int _size;
	int _distance;
	std::bitset<MAX_GRAPH> _contents;

public:
	static void setup(TSPGraph *graph)
	{
		_graph = graph;
		if (_graph->size() > MAX_GRAPH)
			throw std::runtime_error("Graph bigger than MAX_GRAPH");
	}
	static int full() { return _graph->size(); } // the size of a full path

	TSPPath()
	{
		_node[0] = FIRST_NODE;
		_size = 1;
		_distance = 0;
		_contents.reset();
		_contents.set(FIRST_NODE);
	}

	void maximise() { _distance = INT_MAX; }
	int size() { return _size; }
	int distance() { return _distance; }
	bool contains(int i) { return _contents.test(i); }
	int tail() { return _node[_size - 1]; }

	void push(int node)
	{
		if (node >= _graph->size())
			throw std::runtime_error("Node outside graph.");
		_distance += _graph->distance(tail(), node);
		_contents.set(node);
		_node[_size++] = node;
	}

	void pop()
	{
		if (_size < 2)
			throw std::runtime_error("Empty path to pop().");
		_size--;
		int oldtail = _node[_size];
		int newtail = _node[_size - 1];
		if (oldtail != FIRST_NODE)
			_contents.reset(oldtail);
		_distance -= _graph->distance(newtail, oldtail);
	}

	void write(std::ostream &os) const
	{
		os << "[" << _distance << ": ";
		for (int i = 0; i < _size; i++)
		{
			if (i)
				os << ", ";
			os << _node[i];
		}
		os << "]";
	}
};

std::ostream &operator<<(std::ostream &os, const TSPPath &t)
{
	t.write(os);
	return os;
}

class TSPTask : public Task
{

private:
	int _cutoff_size;
	// static TSPPath _shortest;
	static std::atomic<TSPPath *> _best;
	// static std::vector<TSPTask *> _free_list;

	// static TSPTask *alloc(const TSPPath &path, int node)
	// {
	// 	if (_free_list.empty())
	// 		return new TSPTask(path, node);
	// 	TSPTask *p = _free_list.back();
	// 	_free_list.pop_back();
	// 	p->_path = path;
	// 	p->_path.push(node);
	// 	return p;
	// }

	// To be thread-safe
	TSPTask *resusealloc(int node)
	{
		return new TSPTask(this, node);
	}

	// static void free(TSPTask *p)
	// {
	// 	//        	delete p;
	// 	_free_list.push_back(p);
	// }

	// TO be thread-safe
	void reusefree(TSPTask *p)
	{
		delete p;
	}

	TSPPath _path;

	// TSPTask(const TSPPath &path, int node) : _path(path)
	// {
	// 	_path.push(node);
	// }
	TSPTask(TSPTask *task, int node) : _path(task->_path), _cutoff_size(task->_cutoff_size)
	{
		_path.push(node);
	}

public:
	// TSPTask(int cutoff)
	// {
	// 	_shortest.maximise();
	// 	_cutoff_size = TSPPath::full() - cutoff;
	// }
	TSPTask() { _cutoff_size = TSPPath::full(); }
	~TSPTask() override = default;

	// int size()
	// {
	// 	return TSPPath::full();
	// }

	TSPPath &result()
	{
		// return _shortest;
		TSPPath *p = _best.load(std::memory_order_acquire);
		static TSPPath dummy;
		return p ? *p : dummy;
	}

	// Task interface implementation: split, merge, solve, write

	// int split(Task **partitions, int size) override
	// {
	// 	if (_path.size() >= _cutoff_size || size < TSPPath::full())
	// 		return 0;
	// 	int count = 0;
	// 	for (int i = 0; i < TSPPath::full(); i++)
	// 	{
	// 		if (!_path.contains(i))
	// 			//				partitions[count ++] = new TSPTask(_path, i);
	// 			partitions[count++] = TSPTask::alloc(_path, i);
	// 	}
	// 	return count;
	// }

	int split(TaskCollection *collection) override
	{
		if (_path.size() >= _cutoff_size)
			return 0;
		int count = 0;
		for (int i = 0; i < TSPPath::full(); i++)
		{
			if (!_path.contains(i))
			{
				TSPTask *t = resusealloc(i);
				collection->push(t);
				count++;
			}
		}
		return count;
	}

	// void merge(Task **partitions, int size) override
	// {
	// 	for (int p = 0; p < size; p++)
	// 	{
	// 		TSPTask *t = (TSPTask *)partitions[p];
	// 		//			delete t;
	// 		TSPTask::free(t);
	// 		partitions[p] = nullptr;
	// 	}
	// }

	void merge(TaskCollection *collection) override
	{
		for (int p = 0; p < collection->size(); p++)
		{
			TSPTask *t = (TSPTask *)collection->pop();
			reusefree(t);
		}
	}

	// void solve() override
	// {
	// 	//		std::cout << "solving " << _path << "\n";
	// 	if (_path.size() == TSPPath::full())
	// 	{
	// 		_path.push(TSPPath::FIRST_NODE); // last node = first node
	// 		if (_path.distance() < _shortest.distance())
	// 			_shortest = _path;
	// 		_path.pop();
	// 	}
	// 	else
	// 	{
	// 		for (int i = 0; i < TSPPath::full(); i++)
	// 		{
	// 			if (!_path.contains(i))
	// 			{
	// 				_path.push(i);
	// 				if (_path.distance() < _shortest.distance())
	// 					solve();
	// 				_path.pop();
	// 			}
	// 		}
	// 	}
	// }

	void solve() override
	{
		if (_path.size() == TSPPath::full())
		{
			// When all cities are already in the path
			_path.push(TSPPath::FIRST_NODE); // close the visiting loop
			const int d = _path.distance();

			while (true)
			{
				TSPPath *current = _best.load(std::memory_order_acquire);
				const int curr_best_dist = current ? current->distance() : INT_MAX;

				if (d >= curr_best_dist)
					break;

				TSPPath *candidate = new TSPPath(_path);

				if (_best.compare_exchange_weak(current, candidate, std::memory_order_acq_rel, std::memory_order_acquire))
				{
					break;
				}

				delete candidate;
			}
			_path.pop();
			return;
		}
		else
		{
			// When there is missing cities in the path -> explore next cities in an orderly and careful manner
			const int N = TSPPath::full();

			// 1) Build the list of city candidates not already visited
			int candidates[TSPPath::MAX_GRAPH];
			int extra_dists[TSPPath::MAX_GRAPH];
			int m = 0;

			const int base = _path.distance();
			for (int i = 0; i < N; ++i)
			{
				if (!_path.contains(i))
				{
					_path.push(i);
					extra_dists[m] = _path.distance() - base;
					_path.pop();
					candidates[m] = i;
					++m;
				}
			}

			// 2) Sort the candidates from the closest city to the furthest
			for (int a = 1; a < m; ++a)
			{
				int key_node = candidates[a];
				int key_extra_dist = extra_dists[a];
				int b = a - 1;
				while (b >= 0 && extra_dists[b] > key_extra_dist)
				{
					candidates[b + 1] = candidates[b];
					extra_dists[b + 1] = extra_dists[b];
					--b;
				}
				candidates[b + 1] = key_node;
				extra_dists[b + 1] = key_extra_dist;
			}

			// 3) Explore the sorted candidates and
			int best = currentBestDist();
			for (int k = 0; k < m; ++k)
			{
				int next = candidates[k];
				_path.push(next);

				if (_path.distance() < best)
				{
					// pruning
					solve();
					best = currentBestDist();
				}

				_path.pop();
			}
		}
	}

	void write(std::ostream &os) const override
	{
		std::cout << "Task" << _path;
	}

	static int currentBestDist()
	{
		TSPPath *p = _best.load(std::memory_order_acquire);
		return p ? p->distance() : INT_MAX;
	}
};

TSPGraph *TSPPath::_graph;
// int TSPTask::_cutoff_size = INT_MAX;
// TSPPath TSPTask::_shortest;
// std::vector<TSPTask *> TSPTask::_free_list;
inline std::atomic<TSPPath *> TSPTask::_best{nullptr};
