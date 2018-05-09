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
 * @File lights.h
 * @Brief Describes light sources
 */
#pragma once
#include "color.h"
#include "vector.h"
#include "matrix.h"

class Light {
protected:
	Color color;
	float power;
public:
	virtual ~Light() {}
	
	virtual int getNumSamples() = 0;
	
	virtual void getNthSample(int sampleIdx, const Vector& shadePos, Vector& samplePos, Color& color) = 0;
};


class PointLight: public Light {
	Vector pos;
public:
	PointLight(Color color, float power, Vector pos);
	int getNumSamples() { return 1; }
	
	void getNthSample(int sampleIdx, const Vector& shadePos, Vector& samplePos, Color& color) override;
};

class RectLight: public Light {
	Transform T;
	int xSubd, ySubd;
public:
	RectLight(Color color, float power, Transform T, int xSubd = 3, int ySubd = 3);

	int getNumSamples() { return xSubd * ySubd; }
	
	void getNthSample(int sampleIdx, const Vector& shadePos, Vector& samplePos, Color& color) override;
};

extern std::vector<Light*> lights;
