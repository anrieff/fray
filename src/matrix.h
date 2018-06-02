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
 * @File matrix.h
 * @Brief Code to deal with 3x3 real matrices
 */
#pragma once

#include "vector.h"

struct Matrix {
	double m[3][3];
	Matrix() {}
	void makeZero()
	{
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				m[i][j] = 0.0;
	}
	void loadIdentity()
	{
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				m[i][j] = (i == j) ? 1.0 : 0.0;
	}
	Matrix(double diagonalElement)
	{
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				if (i == j) m[i][j] = diagonalElement;
				else m[i][j] = 0.0;
	}
};

inline Vector operator * (const Vector& v, const Matrix& m)
{
	return Vector(
		v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0],
		v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1],
		v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2]
	);
}

inline void operator *= (Vector& v, const Matrix& a) { v = v*a; }

Matrix operator * (const Matrix& a, const Matrix& b); //!< matrix multiplication; result = a*b
Matrix inverseMatrix(const Matrix& a); //!< finds the inverse of a matrix (assuming it exists)
double determinant(const Matrix& a); //!< finds the determinant of a matrix

Matrix rotationAroundX(double angle); //!< returns a rotation matrix around the X axis; the angle is in radians
Matrix rotationAroundY(double angle); //!< same as above, but rotate around Y
Matrix rotationAroundZ(double angle); //!< same as above, but rotate around Z

struct Transform {
	Vector offset;
	Matrix m;
	Matrix invM;
	
	Transform()
	{
		loadIdentity();
	}
	
	// setup the transform:
	void loadIdentity();
	
	void scale(double m) { scale(m, m, m); }
	void scale(double x, double y, double z);
	void rotate(double yaw, double pitch, double roll);
	void translate(const Vector& t);
	
	// use the transform:
	Vector transformPoint(const Vector& t);
	Vector untransformPoint(const Vector& t);
	
	Vector transformDir(const Vector& dir);
	Vector untransformDir(const Vector& dir);
	
	
};
