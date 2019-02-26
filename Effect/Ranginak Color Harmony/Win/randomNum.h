#pragma once

#include <random>
#include <ctime>

namespace randzzz
{

	int randomNum(int min, int max, int seed)
	{
		srand(time(NULL)*time(NULL));
		auto given_seed = std::time(NULL) * rand() * seed;

		std::mt19937 engine(given_seed);
		std::uniform_int_distribution<int> dist(min, max);

		return dist(engine);
	}

}