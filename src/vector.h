/***************************************************************************
 *   Copyright (C) 2009-2018 by Veselin Georgiev, Slavomir Kaslev,         *
 *                              Deyan Hadzhiev et al                       *
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
 * @File vector.h
 * @Brief defines the Vector class (a 3D vector with the usual algebraic operations)
 */ 
#pragma once

#include <math.h>
#include <stdlib.h>

struct Vector {
	union {
		struct { double x, y, z; };
		double v[3];
	};
	
	Vector () {}
	Vector(double _x, double _y, double _z) { set(_x, _y, _z); }
	void set(double _x, double _y, double _z)
	{
		x = _x;
		y = _y;
		z = _z;
	}
	void makeZero(void)
	{
		x = y = z = 0.0;
	}
	bool isZero(void) const
	{
		return x == 0 && y == 0 && z == 0;
	}
	inline double length(void) const
	{
		return sqrt(x * x + y * y + z * z);
	}
	inline double lengthSqr(void) const
	{
		return (x * x + y * y + z * z);
	}
	void scale(double multiplier)
	{
		x *= multiplier;
		y *= multiplier;
		z *= multiplier;
	}
	void operator *= (double multiplier)
	{
		scale(multiplier);
	}
	Vector& operator += (const Vector& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		return *this;
	}
	void operator /= (double divider)
	{
		scale(1.0 / divider);
	}
	void normalize(void)
	{
		double multiplier = 1.0 / length();
		scale(multiplier);
	}
	void setLength(double newLength)
	{
		scale(newLength / length());
	}
	int maxDimension() const
	{
		double maxVal = fabs(x);
		int maxDim = 0;
		if (fabs(y) > maxVal) {
			maxDim = 1;
			maxVal = fabs(y);
		}
		if (fabs(z) > maxVal) {
			maxDim = 2;
		}
		return maxDim;
	}
	inline double& operator[](const int index) { return v[index]; }
	inline const double& operator[](const int index) const { return v[index]; }
};

inline Vector normalize(Vector t)
{
	t.normalize();
	return t;
}

inline Vector operator + (const Vector& a, const Vector& b)
{
	return Vector(a.x + b.x, a.y + b.y, a.z + b.z);
}

inline Vector operator - (const Vector& a, const Vector& b)
{
	return Vector(a.x - b.x, a.y - b.y, a.z - b.z);
}

inline Vector operator - (const Vector& a)
{
	return Vector(-a.x, -a.y, -a.z);
}

/// dot product
inline double operator * (const Vector& a, const Vector& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}
/// dot product (functional form, to make it more explicit):
inline double dot(const Vector& a, const Vector& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}
/// cross product
inline Vector operator ^ (const Vector& a, const Vector& b)
{
	return Vector(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	);
}

inline Vector operator * (const Vector& a, double multiplier)
{
	return Vector(a.x * multiplier, a.y * multiplier, a.z * multiplier);
}
inline Vector operator * (double multiplier, const Vector& a)
{
	return Vector(a.x * multiplier, a.y * multiplier, a.z * multiplier);
}
inline Vector operator / (const Vector& a, double divider)
{
	double multiplier = 1.0 / divider;
	return Vector(a.x * multiplier, a.y * multiplier, a.z * multiplier);
}

inline double distance(const Vector& a, const Vector& b)
{
	Vector dist = a - b;
	return dist.length();
}

// orient the normal, so that we're "outside"
inline Vector faceforward(const Vector& rayDir, const Vector& normal)
{
	if (dot(rayDir, normal) < 0)
		return normal;
	else
		return -normal;
}

// reflect an incoming direction i along normal n (both unit vectors)
inline Vector reflect(const Vector& i, const Vector &n)
{
	return i + 2 * dot(-i, n) * n;
}

// ior = eta1 / eta2
inline Vector refract(const Vector& i, const Vector& n, double ior)
{
	double NdotI = i * n;
	double k = 1 - (ior * ior) * (1 - NdotI * NdotI);
	if (k < 0.0)		// Check for total inner reflection
		return Vector(0, 0, 0);
	return normalize(ior * i - (ior * NdotI + sqrt(k)) * n);
}

// returns b and c, so that:
// dot(a, b) == 0
// dot(a, c) == 0
// dot(b, c) == 0
inline void orthonormalSystem(const Vector& a, Vector& b, Vector& c)
{
	const Vector TEST_VECTORS[2] = {
		{ 1, 0, 0 },
		{ 0, 1, 0 },
	};
	
	Vector testVector = TEST_VECTORS[0];
	
	if (fabs(dot(testVector, a)) > 0.9) 
		testVector = TEST_VECTORS[1];
	
	b = a ^ testVector;
	b.normalize();
	c = a ^ b;
	//c.normalize()
}

// length (x, y) <= 1 (distance to origin is <= 1)
// sqrt(x*x + y*y) <= 1
inline void randomUnitDiscPoint(double& x, double& y)
{
	while (1) {
#if _WIN32
		// FIXME: This is fairly inferior to the drand48() distribution, but should suffice, until
		// the mersene twister RNG support.
		x = double(rand()) / RAND_MAX;
		y = double(rand()) / RAND_MAX;
#else
		x = drand48();
		y = drand48();
#endif

		x = x * 2 - 1;
		y = y * 2 - 1;
		
		if (hypot(x, y) <= 1) return;
	}
}

enum {
	RF_DEBUG = 1,
	
	RF_DIFFUSE = 2,
};

/// @class Ray
struct Ray {
	Vector start;
	Vector dir; // unit vector!
	int depth = 0;
	unsigned flags = 0;
	
	Ray() {}
	Ray(const Vector& start, const Vector& dir): start(start), dir(dir) {}
};
