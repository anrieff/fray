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
 * @File lights.cpp
 * @Brief Describes light sources
 */
#include "lights.h"
#include "random_generator.h"
#include <algorithm>

std::vector<Light*> lights;

void PointLight::getNthSample(int sampleIdx, const Vector& shadePos, Vector& samplePos, Color& color)
{
	samplePos = this->pos;
	color = this->color * this->power;
}

void RectLight::beginFrame(void)
{
	center = T.transformPoint(Vector(0, 0, 0));
	Vector a = T.transformPoint(Vector(-0.5, 0.0, -0.5));
	Vector b = T.transformPoint(Vector( 0.5, 0.0, -0.5));
	Vector c = T.transformPoint(Vector( 0.5, 0.0,  0.5));
	float width = (float) (b - a).length();
	float height = (float) (b - c).length();
	area = width * height; // obtain the area of the light, in world space
}


void RectLight::getNthSample(int sampleIdx, const Vector& shadePos, Vector& samplePos, Color& color)
{
	int column = sampleIdx % xSubd;
	int row = sampleIdx / xSubd;
	
	double areaXsize = 1.0 / xSubd;
	double areaYsize = 1.0 / ySubd;
	
	double areaXstart = column * areaXsize;
	double areaYstart = row * areaYsize;
	
	Random& rnd = getRandomGen();
	
	double p_x = areaXstart + areaXsize * rnd.randfloat();
	double p_y = areaYstart + areaYsize * rnd.randfloat();
	Vector pointOnLight(p_x - 0.5, 0, p_y - 0.5); // ([-0.5..0.5], 0, [-0.5..0.5])
	
	// check if shaded point is behind the lamp:
	Vector shadedPointInLightSpace = T.untransformPoint(shadePos);
	if (shadedPointInLightSpace.y > 0) {
		samplePos.makeZero();
		color.makeZero();
	} else {
		float cosWeight = float(dot(Vector(0, -1, 0), shadedPointInLightSpace) / shadedPointInLightSpace.length());
		color = this->color * power * area * cosWeight;
	}
	
	samplePos = T.transformPoint(pointOnLight);
}

bool RectLight::intersect(Ray ray, IntersectionInfo& info)
{
	Ray rayLocal;
	rayLocal.start = T.untransformPoint(ray.start);
	rayLocal.dir = T.untransformDir(ray.dir);
	
	if (rayLocal.start.y >= 0) return false;
	if (rayLocal.dir.y <= 0) return false;
	
	double travelByY = fabs(rayLocal.start.y);
	double unitTravel = fabs(rayLocal.dir.y);
	double scaling = travelByY / unitTravel;

	info.ip = rayLocal.start + rayLocal.dir * scaling;
	
	if (fabs(info.ip.x) > 0.5 || fabs(info.ip.z) > 0.5) return false;
	
	info.norm = Vector(0, -1, 0);
	
	info.ip = T.transformPoint(info.ip);
	info.norm = T.transformDir(info.norm);
	info.dist = distance(ray.start, info.ip);

	return true;
}

double RectLight::solidAngle(const IntersectionInfo& x) 
{
	return area / std::max(1.0, (x.ip - center).lengthSqr());
}
