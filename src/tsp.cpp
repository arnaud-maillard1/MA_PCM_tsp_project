#include <iostream>
#include "tsptask.hpp"

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " <file.tsp>\n";
		return 1;
	}

	TSPGraph graph(argv[1]);
	TSPPath::setup(&graph);

	TSPTask tsp2(0);
	DirectTaskRunner r2;
	r2.run(&tsp2);
	std::cout << "direct solver: " << tsp2.result() << " time: " << r2.duration() << std::endl;

	TSPTask tsp1(2);
	PartitionedTaskRunner r1(TSPPath::MAX_GRAPH);
	r1.run(&tsp1);
	std::cout << "partitioned: " << tsp1.result() << " time: " << r1.duration() << std::endl;

	return 0;
}
