#include <iostream>
#include "intvecsorttask.hpp"

int main()
{
	IntVecSortTask iv1;
	iv1.randomize(100);
	IntVecSortTask iv2 = iv1;

	PartitionedTaskRunner rr(2);
	rr.run(&iv1);

	DirectTaskRunner sr;
	sr.run(&iv2);

	std::cout << "partitioned: " << rr.duration() << " seconds" << std::endl;
	std::cout << "result: " << iv1 << std::endl;

	std::cout << "direct solver: " << sr.duration() << " seconds" << std::endl;
	std::cout << "result: " << iv2 << std::endl;
}
