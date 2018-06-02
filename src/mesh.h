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
 * @File mesh.h
 * @Brief Contains the Mesh class.
 */
#pragma once
#include <vector>
#include "geometry.h"
#include "shading.h"
#include "vector.h"
#include "triangle.h"
#include "bbox.h"

struct Texture;

struct KDTreeNode {
	Axis axis;

	union {
		struct {
			// if it is a binary node:	
			double splitPos;
			KDTreeNode* children; // an array of 2 KDTreeNode items, respectively a left and a right child
		};
		// if it is a leaf node:
		std::vector<int>* triangles;
	};
	
	KDTreeNode();
	~KDTreeNode();
	void initBinaryNode();
	void initLeafNode(const std::vector<int>& triangles);
	inline bool isLeafNode() const { return axis == Axis::AXIS_NONE; }
};

class Mesh: public Geometry {
protected:
	std::vector<Vector> vertices;
	std::vector<Vector> normals;
	std::vector<Vector> uvs;
	std::vector<Triangle> triangles;
	
	BBox bbox;
	KDTreeNode* kdRoot = nullptr;
	int maxTreeDepth = 0, nodeDepthSum = 0, numNodes = 0;

	void computeBoundingGeometry();
	void prepareTriangles();
	bool intersectTriangle(const Ray& ray, const Triangle& T, IntersectionInfo& info);
	void buildKD(KDTreeNode* node, const std::vector<int>& triangleIndices, BBox bbox, int depth);
	bool intersectKD(const RRay& ray, IntersectionInfo& info, KDTreeNode& node, const BBox& bbox);
public:

	bool faceted = false;
	bool useKD = true;
	bool backfaceCulling = true;

	~Mesh();
	
	void fillProperties(ParsedBlock& pb)
	{
		char fn[256];
		if (pb.getFilenameProp("file", fn)) {
			if (!loadFromOBJ(fn)) {
				pb.signalError("Could not parse OBJ file!");
			}
		} else {
			pb.requiredProp("file");
		}
		pb.getBoolProp("faceted", &faceted);
		pb.getBoolProp("backfaceCulling", &backfaceCulling);
		pb.getBoolProp("useKDTree", &useKD);
	}


	bool loadFromOBJ(const char* filename);

	void beginRender() override;

	bool intersect(const Ray& ray, IntersectionInfo& info) override;
};
