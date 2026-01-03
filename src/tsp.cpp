#include <iostream>

#include "tsptask.hpp"
#include "workstealing.hpp"

int main(int argc, char **argv)
{
	if (argc < 2 || argc > 5)
	{
		std::cerr << "Usage: " << argv[0]
				  << " <file.tsp> [graph_size] [nb_threads] [max_splitted_tasks]\n";
		return 1;
	}

	// Arguments management
	const char *filename = argv[1];

	int graph_size = 0;
	if (argc >= 3)
		graph_size = std::atoi(argv[2]);

	unsigned nb_threads = std::thread::hardware_concurrency();
	if (argc >= 4)
	{
		nb_threads = static_cast<unsigned>(std::atoi(argv[3]));
		if (nb_threads == 0)
			nb_threads = 1;
	}

	size_t max_splitted_tasks = 1;
	if (argc >= 5)
	{
		long long tmp = std::atoll(argv[4]);
		if (tmp > 0)
			max_splitted_tasks = static_cast<size_t>(tmp);
		else
			max_splitted_tasks = 1;
	}

	// Graph creation
	TSPGraph graph(filename);
	if (argc >= 3 && graph_size > 0)
		graph.resize(graph_size); // permit to reduce the number of cities

	TSPPath::setup(&graph);

	TSPTask tsp_direct;
	DirectTaskRunner direct_runner;
	direct_runner.run(&tsp_direct);
	std::cout << "direct solver: " << tsp_direct.result() << " time: " << direct_runner.duration() << std::endl;

	// WorkStealing
	TSPTask tsp_ws;
	WorkStealingRunner ws_runner(nb_threads, max_splitted_tasks);
	ws_runner.run(&tsp_ws);

	double T_par = ws_runner.duration();

	auto &r = tsp_ws.result();
	std::cout << filename << ';'
			  << graph_size << ';'
			  << nb_threads << ';'
			  << max_splitted_tasks << ';'
			  << T_par << ';'
			  << r << ';'
			  << '\n';

	return 0;
}
