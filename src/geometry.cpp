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
 * @File geometry.cpp
 * @Brief Contains implementations of geometry primitives' intersection methods.
 */
#include "geometry.h"
#include "util.h"
#include <algorithm>
using namespace std;

bool Plane::intersect(Ray ray, IntersectionInfo& info)
{
	if (ray.start.y > height && ray.dir.y >= 0) return false;
	if (ray.start.y < height && ray.dir.y <= 0) return false;
	//
	double travelByY = fabs(ray.start.y - height);
	double unitTravel = fabs(ray.dir.y);
	double scaling = travelByY / unitTravel;
	
	info.ip = ray.start + ray.dir * scaling;
	info.dist = distance(ray.start, info.ip);
	info.norm = Vector(0, 1, 0);
	info.u = info.ip.x;
	info.v = info.ip.z;
	//
	return true;
}

bool Sphere::intersect(Ray ray, IntersectionInfo& info)
{
	// p^2 * ray.dir.length^2 + p * (2 * dot(ray.dir, H)) + (H.length^2 - R^2) = 0
	
	Vector H = ray.start - this->O;
	
	double A = 1 /*ray.dir.lengthSqr()*/;
	double B = 2 * dot(ray.dir, H);
	double C = H.lengthSqr() - sqr(this->R);
	
	double Disc = B*B - 4*A*C;
	if (Disc < 0) return false;

	double sqrtDisc = sqrt(Disc);
	double p1 = (-B + sqrtDisc) / (2*A);
	double p2 = (-B - sqrtDisc) / (2*A);
	
	double smaller = min(p1, p2);
	double larger = max(p1, p2);
	
	if (larger < 0) return false;
	double dist = (smaller >= 0) ? smaller : larger;
	
	info.ip = ray.start + ray.dir * dist;
	info.dist = distance(ray.start, info.ip);
	info.norm = info.ip - this->O;
	info.norm.normalize();
	info.u = toDegrees(atan2(info.norm.z, info.norm.x));
	info.v = toDegrees(asin(info.norm.y));
	return true;
}
