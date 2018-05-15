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
 * @File main.cpp
 * @Brief Raytracer main file
 */
#include <math.h>
#include <vector>
#include "util.h"
#include "sdl.h"
#include "color.h"
#include "vector.h"
#include "matrix.h"
#include "camera.h"
#include "geometry.h"
#include "mesh.h"
#include "scene.h"
#include "lights.h"
#include "shading.h"
#include "environment.h"
using namespace std;

Color vfb[VFB_MAX_SIZE][VFB_MAX_SIZE];

bool visible(const Vector& a, const Vector& b)
{
	Ray ray;
	ray.dir = b - a;
	ray.start = a;
	double maxDist = distance(a, b);
	ray.dir.normalize();
	
	for (auto node: nodes) {
		IntersectionInfo info;
		if (node.intersect(ray, info) && info.dist < maxDist) {
			return false;
		}
	}
	
	return true;
}

Color raytrace(Ray ray)
{
	
	if (ray.depth > MAX_TRACE_DEPTH) return Color(0, 0, 0);
	
	Node closestNode;
	IntersectionInfo closestIntersection;
	closestIntersection.dist = 1e99;
	
	for (auto node: nodes) {
		IntersectionInfo info;
		if (node.intersect(ray, info) && info.dist < closestIntersection.dist) {
			closestIntersection = info;
			closestNode = node;
		}
	}
	
	if (closestIntersection.dist >= 1e99) {
		if (env.loaded) return env.getEnvironment(ray.dir);
		else return Color(0, 0, 0);
	}
		
	
	return closestNode.shader->shade(ray, closestIntersection);
}

Color raytrace(double x, double y)
{
	return raytrace(camera.getScreenRay(x, y));
}

void render()
{
	const long long startTicks = getTicks();
	camera.beginFrame();
	
	const double offsets[5][2] = {
		{ 0, 0 }, 
		{ 0.6, 0 },
		{ 0.3, 0.3 },
		{ 0, 0.6 },
		{ 0.6, 0.6 },
	};
	int numAASamples = COUNT_OF(offsets);
	long long lastUpdate = startTicks;
	int pixelSamples = 1;
	if (antialiasing) pixelSamples = numAASamples;
	if (numDOFsamples > 0)
		pixelSamples = numDOFsamples;
	
	for (int y = 0; y < frameHeight(); y++) {
		for (int x = 0; x < frameWidth(); x++) {
			Color avg(0, 0, 0);
			for (int i = 0; i < pixelSamples; i++) {
				Ray ray;
				if (numDOFsamples > 0) {
					ray = camera.getDOFRay(x + randomFloat(), y + randomFloat());
				} else {
					ray = camera.getScreenRay(x + offsets[i][0], y + offsets[i][1]);
				}
				avg += raytrace(ray);
			}
			vfb[y][x] = avg / pixelSamples;
		}
		const long long currentTime = getTicks();
		if (currentTime - lastUpdate > 100) {
			lastUpdate = currentTime;
			displayVFB(vfb);
		}
	}
	unsigned elapsed = getTicks() - startTicks;
	
	printf("Frame took %d ms\n", elapsed);
}

int main(int argc, char** argv)
{
	initGraphics(RESX, RESY);
	setupScene_Forest();
	camera.beginFrame();
	render();
	displayVFB(vfb);
	waitForUserExit();
	closeGraphics();
	printf("Exited cleanly\n");
	return 0;
}
