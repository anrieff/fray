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
 * @File scene.cpp
 * @Brief Scene file parsing, loading and infrastructure
 */

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

Camera camera;
vector<Node> nodes;
CubemapEnvironment env;


 void setupScene_Forest()
{
	Node plane;
	
	Transform lightT;
	lightT.scale(150);
	lightT.translate(Vector(100, 300, -80));
	Light* defaultLight = new RectLight(Color(1, 1, 0.9), 150000, lightT, 3, 3);
	lights.push_back(defaultLight);
	
	env.loadMaps("data/env/forest");
	
	CheckerTexture* checkerBW = new CheckerTexture(Color(0.5, 0.5, 0.5), Color(0.5, 0.5, 0.5));
	BitmapTexture* floorTiles = new BitmapTexture("data/floor.bmp");
	floorTiles->scaling = 1/100.0;
	//BitmapTexture* world = new BitmapTexture("data/world.bmp");
	CheckerTexture* checkerColor = new CheckerTexture(Color(1, 0.5, 0.5), Color(0.5, 1.0, 1.0));
	CheckerTexture* checkerCube = new CheckerTexture(Color(1, 1, 1), Color(0.5, 0.5, 0.5));
	checkerCube->scaling = 6;

	plane.geometry = new Plane(80);
	
	Layered* planeShader = new Layered;
	
	planeShader->addLayer(new Lambert(checkerBW), Color(1, 1, 1));
	//planeShader->addLayer(new Reflection(1), Color(1, 1, 1) * 0.1);
	
	plane.shader = planeShader;
	nodes.push_back(plane);
	
	Mesh* heartGeom = new Mesh;
	Node heartNode;
	Phong* cubeTex = new Phong(new BitmapTexture("data/texture/zar-texture.bmp"));
	heartGeom->loadFromOBJ("data/geom/truncated_cube.obj");
	heartGeom->bumpMap = new BumpTexture("data/texture/zar-bump.bmp");
	heartGeom->bumpMap->bumpIntensity = 10;
	heartGeom->faceted = true;
	heartGeom->beginRender();
	heartNode.geometry = heartGeom;
	Phong* phong = new Phong(checkerCube);
	phong->exponent = 20;
	phong->specularMultiplier = 0.7;
	heartNode.T.scale(5);
	heartNode.T.rotate(90, 0, 0);
	heartNode.T.translate(Vector(-10, 20, 0));

	Mesh* teapot = new Mesh;
	teapot->loadFromOBJ("data/geom/teapot_hires.obj");
	teapot->beginRender();
	teapot->useKD = true;
	Node teapotNode;
	teapotNode.geometry = teapot;
	teapotNode.shader = new Lambert(checkerCube);
	teapotNode.T.scale(20);
	teapotNode.T.rotate(90, 0, 0);
	teapotNode.T.translate(Vector(60, 0, -30));
	nodes.push_back(teapotNode);
	
	// create a glassy shader by using a:
	// layer 0 (bottom): refraction shader, ior = 1.6, opacity = 1
	// layer 1 (top): reflection shader, multiplier 0.95, opacity = fresnel texture with ior = 1.6
	double GLASS_IOR = 1.6;
	Layered* glassShader = new Layered;
	glassShader->addLayer(new Refraction(GLASS_IOR));
	glassShader->addLayer(new Reflection(0.95), Color(1, 1, 1), new FresnelTexture(GLASS_IOR));
	
	heartNode.shader = cubeTex;
	//sphereIndex = int(nodes.size());
	nodes.push_back(heartNode);
	
	Node cube;
	cube.geometry = new Cube(Vector(0, 0, 0), 15);
	cube.T.rotate(toRadians(30), 0, toRadians(60));
	cube.T.translate(Vector(+40, 16, 30));
	cube.shader = new Lambert(checkerColor);
	//cubeIndex = int(nodes.size());
	nodes.push_back(cube);
	
	camera.pos = Vector(0, 60, -120);
	camera.yaw = toRadians(-10);
	camera.pitch = toRadians(-15);
}

void setupScene_DOF()
{
	Light* defaultLight = new PointLight(Color(1, 1, 0.9), 100000, Vector(200, 200, -200));
	lights.push_back(defaultLight);
	
	
	camera.pos = Vector(1.5, 17, -19.5);
	camera.yaw = toRadians(5.2);
	camera.pitch = toRadians(-41.8);
	camera.roll = toRadians(2.3);
	camera.fov = 38;
	camera.aspectRatio = 1.5;
	/*
		dof           on
	focalPlaneDist 25.29
	fNumber       2.0     # This is also not quite correct, the actual aperture was f/2.0
	numSamples      100
	*/
	camera.focalPlaneDist = 29.29;
	camera.fNumber = 2;  // f/2
	
	Mesh* leaf = new Mesh;
	leaf->loadFromOBJ("data/geom/leaf.obj");
	leaf->faceted = true;
	leaf->beginRender();
	
	BitmapTexture* zaphod = new BitmapTexture("data/texture/zaphod.bmp");
	Lambert* lambert = new Lambert(zaphod);
	
	Node paper;
	paper.geometry = leaf;
	paper.shader = lambert;
	paper.T.scale(10);
	nodes.push_back(paper);
}

