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
 * @File vector.h
 * @Brief defines the Vector class (a 3D vector with the usual algebraic operations)
 */ 
#pragma once

#include <math.h>

struct Vector {
	double x, y, z;
	
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
};

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

/// @class Ray
struct Ray {
	Vector start;
	Vector dir; // unit vector!
	
	Ray() {}
	Ray(const Vector& start, const Vector& dir): start(start), dir(dir) {}
};
