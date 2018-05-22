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
#include <string.h>
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
#include "random_generator.h"
using namespace std;

Color vfb[VFB_MAX_SIZE][VFB_MAX_SIZE];
char sceneFile[256] = "data/cornell_box.fray";

bool visible(const Vector& a, const Vector& b)
{
	Ray ray;
	ray.dir = b - a;
	ray.start = a;
	double maxDist = distance(a, b);
	ray.dir.normalize();
	
	for (auto node: scene.nodes) {
		IntersectionInfo info;
		if (node->intersect(ray, info) && info.dist < maxDist) {
			return false;
		}
	}
	
	return true;
}

void applyBumpMapping(Node& closestNode, IntersectionInfo& info)
{
	if (!closestNode.bump) return;
	
	void* interface = closestNode.bump->getInterface(BumpMapperInterface::ID);
	if (interface) {
		static_cast<BumpMapperInterface*>(interface)->modifyNormal(info);
	}
}

Vector hemisphereSample(const IntersectionInfo& info, Vector& resultRay)
{
	// we want unit resultRay (direction), such that dot(info.norm, resultRay) >= 0
	
	Random& rnd = getRandomGen();
	
	double u = rnd.randdouble();
	double v = rnd.randdouble();
	
	double theta = 2 * PI * u;
	double phi = acos(2 * v - 1);
	
	Vector v(
		sin(phi) * cos(theta),
		cos(phi),
		sin(phi) * sin(theta)
	);
	
	// v is uniform in the unit sphere
	
	if (dot(v, info.norm) < 0)
		resultRay = -v;
	else
		resultRay = v;
}

Color pathtrace(Ray ray, Color pathMultiplier)
{
	if (ray.depth > MAX_TRACE_DEPTH ||
		pathMultiplier.intensity() < 0.01 
		)
		return Color(0, 0, 0);
	
	Node* closestNode = nullptr;
	IntersectionInfo closestIntersection;
	closestIntersection.dist = 1e99;
	
	for (auto node: scene.nodes) {
		IntersectionInfo info;
		if (node->intersect(ray, info) && info.dist < closestIntersection.dist) {
			closestIntersection = info;
			closestNode = node;
		}
	}
	
	bool hitLight = false;
	Light* intersectedLight = nullptr;
	for (auto light: scene.lights) {
		IntersectionInfo info;
		if (light->intersect(ray, info) && info.dist < closestIntersection.dist) {
			hitLight = true;
			closestIntersection = info;
			intersectedLight = light;
		}
	}
	
	if (hitLight) {
		return intersectedLight->getColor() * pathMultiplier;
	}
	
	if (!closestNode) {
		if (scene.environment)
			return scene.environment->getEnvironment(ray.dir) * pathMultiplier;
		else
			return Color(0, 0, 0);
	}
		
	applyBumpMapping(*closestNode, closestIntersection);
	
	//return closestNode->shader->shade(ray, closestIntersection);
	
}

Color raytrace(Ray ray)
{
	if (ray.depth > MAX_TRACE_DEPTH) return Color(0, 0, 0);
	
	Node* closestNode = nullptr;
	IntersectionInfo closestIntersection;
	closestIntersection.dist = 1e99;
	
	for (auto node: scene.nodes) {
		IntersectionInfo info;
		if (node->intersect(ray, info) && info.dist < closestIntersection.dist) {
			closestIntersection = info;
			closestNode = node;
		}
	}
	
	bool hitLight = false;
	Light* intersectedLight = nullptr;
	for (auto light: scene.lights) {
		IntersectionInfo info;
		if (light->intersect(ray, info) && info.dist < closestIntersection.dist) {
			hitLight = true;
			closestIntersection = info;
			intersectedLight = light;
		}
	}
	
	if (hitLight) {
		return intersectedLight->getColor();
	}
	
	if (!closestNode) {
		if (scene.environment) return scene.environment->getEnvironment(ray.dir);
		else return Color(0, 0, 0);
	}
		
	applyBumpMapping(*closestNode, closestIntersection);
	
	return closestNode->shader->shade(ray, closestIntersection);
}

Color raytrace(double x, double y)
{
	return raytrace(scene.camera->getScreenRay(x, y));
}

void render()
{
	const long long startTicks = getTicks();
	
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
	if (scene.settings.wantAA) pixelSamples = numAASamples;
	if (scene.camera->dof)
		pixelSamples = max(pixelSamples, scene.camera->numDOFSamples);
	Random& rnd = getRandomGen();
	
	for (int y = 0; y < frameHeight(); y++) {
		for (int x = 0; x < frameWidth(); x++) {
			Color avg(0, 0, 0);
			for (int i = 0; i < pixelSamples; i++) {
				if (scene.camera->stereoSeparation == 0) {
					Ray ray;
					if (scene.camera->dof) {
						ray = scene.camera->getDOFRay(x + rnd.randfloat(), y + rnd.randfloat());
					} else {
						ray = scene.camera->getScreenRay(x + offsets[i][0], y + offsets[i][1]);
					}
					avg += raytrace(ray);
				} else {
					Ray leftRay, rightRay;
					leftRay = scene.camera->getScreenRay(x + offsets[i][0], y + offsets[i][1], CAMERA_LEFT);
					rightRay = scene.camera->getScreenRay(x + offsets[i][0], y + offsets[i][1], CAMERA_RIGHT);
					Color leftColor = raytrace(leftRay);
					Color rightColor = raytrace(rightRay);
					leftColor.adjustSaturation(0.1f);
					rightColor.adjustSaturation(0.1f);
					avg += leftColor * scene.camera->leftMask + rightColor * scene.camera->rightMask;
				}
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

bool parseCmdLine(int argc, char** argv)
{
	if (argc == 1) return true;
	if (argc != 2 || !fileExists(argv[1])) {
		printf("Usage: fray [scene.fray]\n");
		return false;
	} else {
		strcpy(sceneFile, argv[1]);
		return true;
	}
}

int main(int argc, char** argv)
{
	if (!parseCmdLine(argc, argv)) return -1;
	initRandom(42);
	scene.parseScene(sceneFile);
	initGraphics(scene.settings.frameWidth, scene.settings.frameHeight);
	scene.beginRender();
	scene.beginFrame();
	render();
	displayVFB(vfb);
	waitForUserExit();
	closeGraphics();
	printf("Exited cleanly\n");
	return 0;
}
