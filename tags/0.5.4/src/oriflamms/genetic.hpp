/*! © 2015 LIPADE
 * \file	genetic.hpp
 * \author	Yann LEYDIER
 */
#include <iterator>
#include <map>
#include <stdexcept>
#include <random>
#include <algorithm>

/////////////////////////////////////////////////////////////////////
// Genetic algorithm
/////////////////////////////////////////////////////////////////////

#include <iostream>
/*!
 * \param[in]	b	iterator on the first individual
 * \param[in]	e	iterator after the last individual
 * \param[in]	breed	a function to create two individuals from two parents: std::pair<GENOTYPE, GENOTYPE> breed(const GENOTYPE &, const GENOTYPE &, URNG &rng)
 * \param[in]	evaluate	a function to evaluate the fitness of an individual: double evaluate(const GENOTYPE &)
 * \param[in]	stop	a function to verify if the algorithm has found a satisfying solution: bool stop(const std::multimap<double, GENOTYPE> &population)
 * \param[in]	rng	a random number generator (default value is the default random generator initialized with current time)
 * \throws	logic_error	there is less than 2 individuals
 * \return	the population, sorted by fitness
 */
template <typename ITER,
				 typename BREEDING_FUNC,
				 typename EVAL_FUNC,
				 typename STOP_FUNC,
				 typename URNG = std::default_random_engine>
std::multimap<double, typename std::iterator_traits<ITER>::value_type> Genetic(ITER b, ITER e,
		BREEDING_FUNC breed,
		EVAL_FUNC evaluate,
		STOP_FUNC stop,
		URNG &&rng = std::default_random_engine{size_t(std::chrono::system_clock::now().time_since_epoch().count())})
{
	using GENOTYPE = typename std::iterator_traits<ITER>::value_type;

	// evaluate the initial population
	auto population = std::multimap<double, GENOTYPE>{};
	for (; b < e; ++b)
		population.emplace(evaluate(*b), *b);
	if (population.size() < 2)
		throw std::logic_error("Parthenogenesis is not allowed, at least two individuals are needed.");

	// iterate generations
	while (!stop(population))
	{
		auto newpopulation = std::multimap<double, GENOTYPE>{};
		//auto it = population.cbegin();
		// create a randomized order of the population
		auto ranpos = std::vector<size_t>(population.size());
		std::iota(ranpos.begin(), ranpos.end(), 0);
		std::shuffle(ranpos.begin(), ranpos.end(), rng);
		// breeding loop
		for (size_t tmp = 0; tmp < population.size(); )
		{
			if (tmp + 1 >= ranpos.size()) // if the number of individuals is odd, the least fitting individual is not bred :o(
				break;
			// pick two random individuals
			auto it1 = std::next(population.begin(), ranpos[tmp++]);
			auto it2 = std::next(population.begin(), ranpos[tmp++]);
			auto children = std::pair<GENOTYPE, GENOTYPE>{};
			if (tmp >= ranpos.size())
			{ // if cannot pick a third, just breed what we have since they are the last remaining individuals.
				children = breed(it1->second, it2->second, rng);
			}
			else
			{ // pick a third random individual
				auto it3 = std::next(population.begin(), ranpos[tmp]);
				// breed the best two, the least fitting one will be part of next love triangle
				if (it1->first < it2->first)
				{
					if (it2->first < it3->first)
					{
						children = breed(it1->second, it2->second, rng);
						// third individual's index stays in place in the list to be reused in next loop
					}
					else
					{
						children = breed(it1->second, it3->second, rng);
						ranpos[tmp] = ranpos[tmp - 1]; // second individual's index stored for next loop
					}
				}
				else
				{
					if (it1->first < it3->first)
					{
						children = breed(it1->second, it2->second, rng);
						// third individual's index stays in place in the list to be reused in next loop
					}
					else
					{
						children = breed(it2->second, it3->second, rng);
						ranpos[tmp] = ranpos[tmp - 2]; // first individual's index stored for next loop
					}
				}
			}

			// evaluate children
			auto fitness = evaluate(children.first);
			newpopulation.emplace(fitness, std::move(children.first));
			fitness = evaluate(children.second);
			newpopulation.emplace(fitness, std::move(children.second));
		} // breeding loop

		/*
		// select the next generation
		const auto s = population.size();
		// move parents to children list
		std::move(population.begin(), population.end(), std::inserter(newpopulation, newpopulation.end()));
		population.clear();
		auto probas = std::map<double, size_t>{}; // probability to choose an individual
		auto lastproba = 0.0;
		auto pos = size_t(0);
		const auto maxval = double(newpopulation.size()) + 1;
		//const auto maxval = newpopulation.rbegin()->first + 1; // used to reverse fitness (low fitness = high probability)
		for (auto &idv : newpopulation)
		{
			//lastproba += maxval - idv.first;
			lastproba += sqrt(maxval - pos);
			probas.emplace(lastproba, pos++);
		}
		// randomly select individuals
		auto selected = std::set<size_t>{};
		//selected.insert(0); // always add the best individual
		auto ran = std::uniform_real_distribution<double>{0.0, lastproba};
		static auto rng2 = std::default_random_engine{0}; // XXX temporary tweak
		while (selected.size() != s)
		{
			const auto sit = probas.upper_bound(ran(rng2));
			selected.insert(sit->second);
		}
		// move selected individuals to the new population
		for (auto r : selected)
		{
			auto sit = std::next(newpopulation.begin(), r);
			population.emplace(sit->first, std::move(sit->second));
		}
		*/
		//const auto s = population.size();
		//std::move(newpopulation.begin(), newpopulation.end(), std::inserter(population, population.end()));
		//population.erase(std::next(population.begin(), s), population.end());
		newpopulation.insert(*population.begin());
		newpopulation.erase(--newpopulation.end());
		population.swap(newpopulation);

		//std::cout << population.begin()->first << std::endl;
		/*
		auto mean = 0.0;
		for (auto &p : population)
			mean += p.first;
		mean /= double(population.size());
		std::cout << mean << std::endl;
		*/
	}
	//std::cout << population.begin()->first << std::endl;
	return population;
}

/////////////////////////////////////////////////////////////////////
// Breeding functors
/////////////////////////////////////////////////////////////////////

/*! \brief Crossover functor
 *
 * Creates two vectors from two parent vectors using complementary random strings from each one.
 */
struct CrossOver
{
	/*!
	 * \param[in]	idv1	first parent
	 * \param[in]	idv2	second parent
	 * \throws	length_error	parents do not have the same size
	 * \throws	invalid_argument	parents are empty
	 * \return	a pair of children
	 */
	template<typename T, typename URNG> inline std::pair<std::vector<T>, std::vector<T>> operator()(const std::vector<T> &idv1, const std::vector<T> &idv2, URNG &rng) const
	{
		if (idv1.size() != idv2.size())
			throw std::length_error("The individuals must have the same size.");
		const auto s = idv1.size();
		if (!s)
			throw std::invalid_argument("The individuals must not be empty.");

		auto ran = std::uniform_int_distribution<size_t>(0, s - 1);
		const auto cut = ran(rng); // position of the cut
		auto child1 = std::vector<T>(s);
		auto child2 = std::vector<T>(s);
		for (auto tmp = size_t(0); tmp < cut; ++tmp)
		{ // copy first string
			child1[tmp] = idv1[tmp];
			child2[tmp] = idv2[tmp];
		}
		for (auto tmp = cut; tmp < s; ++tmp)
		{ // copy second string from the other parent
			child1[tmp] = idv2[tmp];
			child2[tmp] = idv1[tmp];
		}
		return std::make_pair(std::move(child1), std::move(child2));
	}
};

/////////////////////////////////////////////////////////////////////
// Stop functors
/////////////////////////////////////////////////////////////////////

/*! \brief Simple counter to stop an genetic algorithm
 */
struct GenerationCounter
{
	constexpr GenerationCounter(size_t cnt):generation(cnt) {}
	template<typename T> inline bool operator()(const T &) { return --generation == 0; }
	private:
		size_t generation;
};

/*! \brief Stops when the best individual has a fitness lower than a threshold
 */
struct FitnessThreshold
{
	constexpr FitnessThreshold(double thresh):threshold(thresh) {}
	template<typename T> inline bool operator()(const std::multimap<double, T> &population) const { return population.begin()->first < threshold; }
	private:
		size_t threshold;
};

