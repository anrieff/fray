/***************************************************************************
 *   Copyright (C) 2009-2018 by Veselin Georgiev, Slavomir Kaslev et al    *
 *   admin@raytracing-bg.net                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/**
 * @File random_generator.h
 * @Brief The Random class (random generation utilities)
 */
#pragma once

#include <random>

/**
 * @File random_generator.h
 * @Brief holds the Random class, and some functions to fetch random number generators
 *
 * The Random class is based on the high-quality mt19937 (Mersenne Twister)
 * pseudo-random number generator, using the C++11 implementation
 *
 * The Random class is not intended to be created and used directly.
 * Its construction is costly; instead, use one of the getRandomGen() functions.
 */
 
 class Random {
	std::mt19937 generator; // mersenne twister generator
public:
	Random(unsigned seed = 123u);
	void seed(unsigned seed);
	unsigned _next(void); // returns a raw 32-bit unbiased random integer
	int randint(int a, int b); // returns a random integer in [a..b] (a and b can be negative as well)
	float randfloat(void); // return a floating-point number in [0..1)
	double randdouble(void); // same as randfloat(), but in double precision (using two _next() invocations)
	double gaussian(double mean = 0.0, double sigma = 1.0); // return a random number in normal distribution
	void unitDiscSample(double& x, double &y); // get a random point in the unit disc (x*x + y*y <= 1)
};
 
/// seed the whole array of random generators.
void initRandom(unsigned seed);

/// fetch the idx-th random generator. There are at least 250 random generators, which are prepared and ready.
/// This function does not take any start-up time and should be very fast.
Random& getRandomGen(int idx);

/// fetch a fixed random generator, based on the calling thread's ID. I.e., within each thread, all calls to getRandomGen()
/// are guaranteed to return the same object; in the same time, different threads get different random generators
/// thus no locking is required, and no performance degradation can occur.
Random& getRandomGen(void);
