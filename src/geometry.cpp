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
	
	Vector ip = ray.start + ray.dir * scaling;
	if (fabs(ip.x) > limit) return false;
	if (fabs(ip.z) > limit) return false;
	info.ip = ip;
	info.dist = distance(ray.start, info.ip);
	info.norm = Vector(0, 1, 0);
	info.u = info.ip.x;
	info.v = info.ip.z;
	info.geom = this;
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
	info.u = (toDegrees(atan2(info.norm.z, info.norm.x)) + 180.0) / 360.0;
	info.v = 1 - (toDegrees(asin(info.norm.y)) + 90) / 180.0;
	info.geom = this;
	return true;
}

void Cube::intersectCubeSide(Ray ray, double start, double dir, double target, const Vector& normal,
							IntersectionInfo& info, std::function<void (const Vector&)> uv_mapping)
{
	if (fabs(dir) < 1e-9) return;

	// start + mult*dir = target
	double mult = (target - start) / dir;
	
	if (mult < 0) return;
	
	Vector ip = ray.start + ray.dir * mult;
	
	if (ip.x < O.x - halfSide - 1e-6 || ip.x > O.x + halfSide + 1e-6) return;
	if (ip.y < O.y - halfSide - 1e-6 || ip.y > O.y + halfSide + 1e-6) return;
	if (ip.z < O.z - halfSide - 1e-6 || ip.z > O.z + halfSide + 1e-6) return;
		
	double dist = distance(ray.start, ip);
	if (dist < info.dist) {
		info.dist = dist;
		info.ip = ip;
		
		info.norm = normal;
		uv_mapping(ip);
	}
}

bool Cube::intersect(Ray ray, IntersectionInfo& info)
{
	info.dist = 1e99;
	
	auto sideX_UV = [&info] (const Vector& ip) { info.u = ip.y; info.v = ip.z; };
	auto sideY_UV = [&info] (const Vector& ip) { info.u = ip.x; info.v = ip.z; };
	auto sideZ_UV = [&info] (const Vector& ip) { info.u = ip.x; info.v = ip.y; };

	// X:	
	intersectCubeSide(ray, ray.start.x, ray.dir.x, O.x - halfSide, Vector(-1, 0, 0), info, sideX_UV);
	intersectCubeSide(ray, ray.start.x, ray.dir.x, O.x + halfSide, Vector(+1, 0, 0), info, sideX_UV);

	// Y:
	intersectCubeSide(ray, ray.start.y, ray.dir.y, O.y - halfSide, Vector(0, -1, 0), info, sideY_UV);
	intersectCubeSide(ray, ray.start.y, ray.dir.y, O.y + halfSide, Vector(0, +1, 0), info, sideY_UV);
	
	// Z:
	intersectCubeSide(ray, ray.start.z, ray.dir.z, O.z - halfSide, Vector(0, 0, -1), info, sideZ_UV);
	intersectCubeSide(ray, ray.start.z, ray.dir.z, O.z + halfSide, Vector(0, 0, +1), info, sideZ_UV);

	if (info.dist < 1e99) {
		info.geom = this;
		return true;
	} else {
		return false;
	}
}

vector<IntersectionInfo> findAllIntersections(Ray ray, Geometry* g)
{
	vector<IntersectionInfo> result;
	
	int counter = 30;
	Vector origin = ray.start;
	
	IntersectionInfo info;
	while (g->intersect(ray, info) && counter --> 0) {
		result.push_back(info);
	
		ray.start = info.ip + ray.dir * 1e-6;
	}
	
	for (int i = 1; i < int(result.size()); i++)
		result[i].dist = distance(result[i].ip, origin);
	
	return result;
}

bool CsgOp::intersect(Ray ray, IntersectionInfo& info)
{
	vector<IntersectionInfo>
		leftIntersections = findAllIntersections(ray, left),
		rightIntersections = findAllIntersections(ray, right);
	
	vector<IntersectionInfo> allIntersections;
	
	for (auto& ip: leftIntersections) allIntersections.push_back(ip);
	for (auto& ip: rightIntersections) allIntersections.push_back(ip);
	
	sort(allIntersections.begin(), allIntersections.end(), []
		(const IntersectionInfo& a, const IntersectionInfo& b) -> bool {
			return a.dist < b.dist;
	});
	
	bool inLeft = (leftIntersections.size() % 2) == 1;
	bool inRight = (rightIntersections.size() % 2) == 1;
	
	bool boolResult = boolOp(inLeft, inRight);
	
	for (auto& ip: allIntersections) {
		if (ip.geom == left) inLeft = !inLeft;
		else                 inRight = !inRight;
		bool newBoolResult = boolOp(inLeft, inRight);
		
		if (newBoolResult != boolResult) {
			info = ip;
			info.geom = this;
			return true;
		}
	}
	
	return false;
}

bool Node::intersect(Ray ray, IntersectionInfo& info)
{
	Ray localRay = ray;
	localRay.start = T.untransformPoint(ray.start);
	localRay.dir = T.untransformDir(ray.dir);
	
	if (!geometry->intersect(localRay, info)) return false;
	
	info.ip = T.transformPoint(info.ip);
	info.norm = T.transformDir(info.norm);
	info.dist = distance(ray.start, info.ip);
	return true;
}
