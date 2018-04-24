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
 * @File mesh.h
 * @Brief Contains the Mesh class.
 */
#pragma once
#include <vector>
#include "geometry.h"
#include "shading.h"
#include "vector.h"

/// A structure to represent a single triangle in the mesh
struct Triangle {
	int v[3]; //!< holds indices to the three vertices of the triangle (indexes in the `vertices' array in the Mesh)
	int n[3]; //!< holds indices to the three normals of the triangle (indexes in the `normals' array)
	int t[3]; //!< holds indices to the three texture coordinates of the triangle (indexes in the `uvs' array)
	Vector gnormal; //!< The geometric normal of the mesh (AB ^ AC, normalized)
	Vector dNdx, dNdy; //!< tangent and binormal vectors for this triangle
};

struct Texture;

class Mesh: public Geometry {
protected:
	std::vector<Vector> vertices;
	std::vector<Vector> normals;
	std::vector<Vector> uvs;
	std::vector<Triangle> triangles;
	
	Sphere boundingSphere;

	void computeBoundingGeometry();
	void prepareTriangles();
	bool intersectSingleTriangle(Ray ray, const Triangle& T,
		     					 double& minDist,
								 double& l2, double& l3);
public:

	bool faceted = false;
	bool backfaceCulling = true;
	BumpTexture* bumpMap = nullptr;

	~Mesh();

	bool loadFromOBJ(const char* filename);

	void beginRender();

	bool intersect(Ray ray, IntersectionInfo& info) override;
};
