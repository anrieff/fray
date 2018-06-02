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
 * @File lights.h
 * @Brief Describes light sources
 */
#pragma once
#include "color.h"
#include "vector.h"
#include "matrix.h"
#include "scene.h"
#include "geometry.h"

class Light: public SceneElement, public Intersectable {
protected:
	Color color = Color(1, 1, 1);
	float power = 1;
public:
	virtual ~Light() {}
	
	ElementType getElementType() const { return ELEM_LIGHT; }

	virtual int getNumSamples() = 0;
	
	virtual void getNthSample(int sampleIdx, const Vector& shadePos, Vector& samplePos, Color& color) = 0;
	
	virtual Color getColor() { return color * power; }
	
	virtual double solidAngle(const IntersectionInfo& x) { return 0; }

	void fillProperties(ParsedBlock& pb)
	{
		pb.getColorProp("color", &color);
		pb.getFloatProp("power", &power);
	}
};


class PointLight: public Light {
	Vector pos = Vector(0, 0, 0);
public:
	void fillProperties(ParsedBlock& pb)
	{
		Light::fillProperties(pb);
		pb.getVectorProp("pos", &pos);
	}

	int getNumSamples() { return 1; }
	
	bool intersect(const Ray& ray, IntersectionInfo& info) override
	{
		return false;
	}
	
	void getNthSample(int sampleIdx, const Vector& shadePos, Vector& samplePos, Color& color) override;
};

class RectLight: public Light {
	Transform T;
	int xSubd, ySubd;
	Vector center;
	double area;
public:
	void fillProperties(ParsedBlock& pb)
	{
		Light::fillProperties(pb);
		pb.getIntProp("xSubd", &xSubd, 1);
		pb.getIntProp("ySubd", &ySubd, 1);
		pb.getTransformProp(T);
	}
	
	void beginFrame();

	int getNumSamples() { return xSubd * ySubd; }
	
	void getNthSample(int sampleIdx, const Vector& shadePos, Vector& samplePos, Color& color) override;

	bool intersect(const Ray& ray, IntersectionInfo& info) override;
	
	double solidAngle(const IntersectionInfo& x) override;
};
