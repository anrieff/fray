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
 * @File mesh.cpp
 * @Brief Contains implementation of the Mesh class
 */

#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <numeric>
#include <SDL/SDL.h>
#include "mesh.h"
#include "constants.h"
#include "color.h"
#include "bbox.h"
using std::max;
using std::vector;
using std::string;


void Mesh::beginRender()
{
	computeBoundingGeometry();
}


void Mesh::computeBoundingGeometry()
{
//	boundingSphere.O.makeZero();
//	double R = 0;
//	for (auto& vert: vertices) {
//		R = max(R, distance(vert, boundingSphere.O));
//	}
//	boundingSphere.R = R;
	bbox.makeEmpty();
	for (auto& vert: vertices) {
		bbox.add(vert);
	}
	
	vector<int> allTriangles;
	for (int i = 0; i < int(triangles.size()); i++)
		allTriangles.push_back(i);
	
	unsigned start = SDL_GetTicks();
	kdRoot = new KDTreeNode;
	buildKD(kdRoot, allTriangles, bbox, 0);
	unsigned end = SDL_GetTicks();
	
	printf("KD Tree for %d triangles built in %u milliseconds (%d nodes, max depth = %d, avg depth = %.1f)\n", 
			int(triangles.size()), end - start, numNodes, maxTreeDepth, nodeDepthSum / float(numNodes));
}

Mesh::~Mesh()
{
}

bool Mesh::intersectTriangle(const Ray& ray, const Triangle& T, IntersectionInfo& info)
{
	double lambda2, lambda3;
	// backface culling?
	if (backfaceCulling && dot(ray.dir, T.gnormal) > 0) return false;
	const Vector& A = vertices[T.v[0]];
	const Vector& B = vertices[T.v[1]];
	const Vector& C = vertices[T.v[2]];

	if (T.intersect(ray, A, B, C, info.dist, lambda2, lambda3)) {
		info.geom = this;
		info.ip = ray.start + ray.dir * info.dist;
		if (faceted || normals.empty()) {
			info.norm = T.gnormal;
		} else {
			const Vector& nA = normals[T.n[0]];
			const Vector& nB = normals[T.n[1]];
			const Vector& nC = normals[T.n[2]];
			
			info.norm = nA + (nB - nA) * lambda2 + (nC - nA) * lambda3;
			info.norm.normalize();
		}
		
		if (uvs.empty()) {
			info.u = info.v = 0;
		} else {
			const Vector& tA = uvs[T.t[0]];
			const Vector& tB = uvs[T.t[1]];
			const Vector& tC = uvs[T.t[2]];
			
			Vector texCoord = tA + (tB - tA) * lambda2 + (tC - tA) * lambda3;
			info.u = texCoord.x;
			info.v = texCoord.y;
			
			if (bumpMap) {
				float dx, dy;
				bumpMap->getDeflection(info, dx, dy);
				info.norm += dx * T.dNdx + dy * T.dNdy;
				info.norm.normalize();
			}				
		}
		return true;
	}
	
	return false;
}


bool Mesh::intersect(Ray ray, IntersectionInfo& info)
{
	if (!bbox.testIntersect(ray))
		return false;
	
	info.dist = INF;
	bool found = false;
	
	if (useKD && kdRoot) {
		found = intersectKD(ray, info, kdRoot, bbox);
	} else {
		for (auto& T: triangles) {
			if (intersectTriangle(ray, T, info)) {
				found = true;
			}
		}
	}
	
	return found;
}

static int toInt(const string& s)
{
	if (s.empty()) return 0;
	int x;
	if (1 == sscanf(s.c_str(), "%d", &x)) return x;
	return 0;
}

static double toDouble(const string& s)
{
	if (s.empty()) return 0;
	double x;
	if (1 == sscanf(s.c_str(), "%lf", &x)) return x;
	return 0;
}

static void parseTrio(string s, int& vertex, int& uv, int& normal)
{
	vector<string> items = split(s, '/');
	// "4" -> {"4"} , "4//5" -> {"4", "", "5" }

	vertex = toInt(items[0]);
	uv = items.size() >= 2 ? toInt(items[1]) : 0;
	normal = items.size() >= 3 ? toInt(items[2]) : 0;
}

static Triangle parseTriangle(string s0, string s1, string s2)
{
	// "3", "3/4", "3//5", "3/4/5"  (v/uv/normal)
	Triangle T;
	parseTrio(s0, T.v[0], T.t[0], T.n[0]);
	parseTrio(s1, T.v[1], T.t[1], T.n[1]);
	parseTrio(s2, T.v[2], T.t[2], T.n[2]);
	return T;
}

bool Mesh::loadFromOBJ(const char* filename)
{
	FILE* f = fopen(filename, "rt");

	if (!f) return false;

	vertices.push_back(Vector(0, 0, 0));
	uvs.push_back(Vector(0, 0, 0));
	normals.push_back(Vector(0, 0, 0));

	char line[10000];

	while (fgets(line, sizeof(line), f)) {
		if (line[0] == '#') continue;

		vector<string> tokens = tokenize(line);
		// "v 0 1    4" -> { "v", "0", "1", "4" }

		if (tokens.empty()) continue;

		if (tokens[0] == "v")
			vertices.push_back(
				Vector(
					toDouble(tokens[1]),
					toDouble(tokens[2]),
					toDouble(tokens[3])));

		if (tokens[0] == "vn")
			normals.push_back(
				Vector(
					toDouble(tokens[1]),
					toDouble(tokens[2]),
					toDouble(tokens[3])));

		if (tokens[0] == "vt")
			uvs.push_back(
				Vector(
					toDouble(tokens[1]),
					toDouble(tokens[2]),
					0));

		if (tokens[0] == "f") {
			for (int i = 0; i < int(tokens.size()) - 3; i++) {
				triangles.push_back(
					parseTriangle(tokens[1], tokens[2 + i], tokens[3 + i])
				);
			}
		}
	}

	fclose(f);

	prepareTriangles();
	return true;
}


static void solve2D(Vector A, Vector B, Vector C, double& x, double& y)
{
	// solve: x * A + y * B = C
	double mat[2][2] = { { A.x, B.x }, { A.y, B.y } };
	double h[2] = { C.x, C.y };

	double Dcr = mat[0][0] * mat[1][1] - mat[1][0] * mat[0][1];
	x =         (     h[0] * mat[1][1] -      h[1] * mat[0][1]) / Dcr;
	y =         (mat[0][0] *      h[1] - mat[1][0] *      h[0]) / Dcr;
}


void Mesh::prepareTriangles()
{
	
	for (auto& t: triangles) {
		Vector A = vertices[t.v[0]];
		Vector B = vertices[t.v[1]];
		Vector C = vertices[t.v[2]];
		Vector AB = B - A;
		Vector AC = C - A;
		t.gnormal = AB ^ AC;
		t.gnormal.normalize();
		
		if (!uvs.empty() && !normals.empty()) {
			Vector tA = uvs[t.t[0]];
			Vector tB = uvs[t.t[1]];
			Vector tC = uvs[t.t[2]];

			Vector tAB = tB - tA;
			Vector tAC = tC - tA;
			
			double px, qx, py, qy;
			solve2D(tAB, tAC, Vector(1, 0, 0), px, qx);
			// px * tAB + qx * tAC = (1, 0, 0)
			solve2D(tAB, tAC, Vector(0, 1, 0), py, qy);
			// py * tAB + qy * tAC = (0, 1, 0)
			
			t.dNdx = px * AB + qx * AC;
			t.dNdy = py * AB + qy * AC;
			t.dNdx.normalize();
			t.dNdy.normalize();
		} else {
			t.dNdx.makeZero();
			t.dNdy.makeZero();
		}
	}
	
	printf("Mesh loaded, %d triangles\n", int(triangles.size()));
}

inline double findOptimalSplitPlane(const vector<int>& triangleIndices, BBox bbox, Axis axis)
{
	return (bbox.vmin[int(axis)] + bbox.vmax[int(axis)]) * 0.5; // <- this could be improved a lot!
}

void Mesh::buildKD(KDTreeNode*& node, const vector<int>& triangleIndices, BBox bbox, int depth)
{
	numNodes++;
	maxTreeDepth = max(maxTreeDepth, depth);
	if (int(triangleIndices.size()) <= MAX_TRIANGLES_PER_LEAF || depth > MAX_DEPTH) {
		// make a leaf node:
		node->axis = Axis::AXIS_NONE;
		node->left = node->right = nullptr;
		
		node->triangles = triangleIndices;
		nodeDepthSum += depth;
		return;
	}
	
	node->axis = Axis(depth % 3);
	node->splitPos = findOptimalSplitPlane(triangleIndices, bbox, node->axis);
	
	BBox leftbbox, rightbbox;
	bbox.split(node->axis, node->splitPos, leftbbox, rightbbox);
	
	vector<int> leftTriangles, rightTriangles;
	for (auto& ti: triangleIndices) {
		Triangle& T = triangles[ti];
		const Vector& A = vertices[T.v[0]];
		const Vector& B = vertices[T.v[1]];
		const Vector& C = vertices[T.v[2]];
		
		if (leftbbox.intersectTriangle(A, B, C))
			leftTriangles.push_back(ti);
		
		if (rightbbox.intersectTriangle(A, B, C))
			rightTriangles.push_back(ti);
	}
	
	node->left = new KDTreeNode;
	node->right = new KDTreeNode;
	buildKD(node->left, leftTriangles, leftbbox, depth + 1);
	buildKD(node->right, rightTriangles, rightbbox, depth + 1);
	nodeDepthSum += depth;
}

bool Mesh::intersectKD(Ray ray, IntersectionInfo& info, KDTreeNode* node, const BBox& bbox)
{
	// is it leaf?
	if (node->axis == Axis::AXIS_NONE) {
		bool found = false;
		for (int idx: node->triangles) {
			Triangle& T = this->triangles[idx];
			if (intersectTriangle(ray, T, info))
				found = true;
		}
		
		return found && bbox.inside(info.ip);
	}
	
	// binary node:
	BBox childBBoxen[2];
	
	bbox.split(node->axis, node->splitPos, childBBoxen[0], childBBoxen[1]);
	
	int axis = int(node->axis);
	
	int traverseOrder[2];
	if (ray.start[axis] < node->splitPos) {
		traverseOrder[0] = 0;
		traverseOrder[1] = 1;
	} else {
		traverseOrder[0] = 1;
		traverseOrder[1] = 0;
	}
	
	for (int i = 0; i < 2; i++) {
		int id = traverseOrder[i];
		BBox& child = childBBoxen[id];
		
		if (child.testIntersect(ray)) {
			if (intersectKD(ray, info, id == 0 ? node->left : node->right, child))
				return true;
		}
	}
	return false;
}
