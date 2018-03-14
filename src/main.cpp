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
#include <SDL/SDL.h>
#include <math.h>
#include <vector>
#include "util.h"
#include "sdl.h"
#include "color.h"
#include "vector.h"
#include "camera.h"
#include "geometry.h"
#include "shading.h"
using namespace std;

Color vfb[VFB_MAX_SIZE][VFB_MAX_SIZE];

struct Node {
	Geometry* geometry;
	Shader* shader;
};

Camera camera;
vector<Node> nodes;

void setupScene()
{
	Node plane;
	plane.geometry = new Plane;
	plane.shader = new CheckerShader;
	nodes.push_back(plane);
	
	Node sphere;
	sphere.geometry = new Sphere(Vector(-10, 60, 0), 30);
	sphere.shader = new CheckerShader(Color(1, 0.5, 0.5), Color(0.5, 1.0, 1.0));
	nodes.push_back(sphere);
	
	camera.pos = Vector(0, 60, -120);
	camera.yaw = toRadians(-10);
	camera.pitch = toRadians(-15);
}

bool visible(const Vector& a, const Vector& b)
{
	Ray ray;
	ray.dir = b - a;
	ray.start = a;
	double maxDist = distance(a, b);
	ray.dir.normalize();
	
	for (auto node: nodes) {
		IntersectionInfo info;
		if (node.geometry->intersect(ray, info) && info.dist < maxDist) {
			return false;
		}
	}
	
	return true;
}

Color raytrace(double x, double y)
{
	Ray ray = camera.getScreenRay(x, y);
	
	Node closestNode;
	IntersectionInfo closestIntersection;
	closestIntersection.dist = 1e99;
	
	for (auto node: nodes) {
		IntersectionInfo info;
		if (node.geometry->intersect(ray, info) && info.dist < closestIntersection.dist) {
			closestIntersection = info;
			closestNode = node;
		}
	}
	
	if (closestIntersection.dist >= 1e99)
		return Color(0, 0, 0);
	
	return closestNode.shader->shade(ray, closestIntersection);
}

void render()
{
	camera.beginFrame();
	for (int y = 0; y < frameHeight(); y++) {
		for (int x = 0; x < frameWidth(); x++) {
			vfb[y][x] = raytrace(x, y);
		}
	}
}

int main(int argc, char** argv)
{
	initGraphics(RESX, RESY);
	setupScene();
	//for (double angle = 0; angle < 360; angle += 15) {
		//camera.fov = fov;
//		extern Vector lightPos;
//		double angleRad = toRadians(angle);
//		lightPos = Vector(cos(angleRad) * 200, 200, sin(angleRad) * 200);
		render();
		displayVFB(vfb);
	//}
	waitForUserExit();
	closeGraphics();
	printf("Exited cleanly\n");
	return 0;
}
