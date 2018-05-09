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

std::vector<Light*> lights;

PointLight::PointLight(Color color, float power, Vector pos)
{
	this->color = color;
	this->power = power;
	this->pos = pos;
}

void PointLight::getNthSample(int sampleIdx, const Vector& shadePos, Vector& samplePos, Color& color)
{
	samplePos = this->pos;
	color = this->color * this->power;
}


RectLight::RectLight(Color color, float power, Transform T, int xSubd, int ySubd)
{
	this->color = color;
	this->power = power;
	this->T = T;
	this->xSubd = xSubd;
	this->ySubd = ySubd;
}

void RectLight::getNthSample(int sampleIdx, const Vector& shadePos, Vector& samplePos, Color& color)
{
	int column = sampleIdx % xSubd;
	int row = sampleIdx / xSubd;
	
	double areaXsize = 1.0 / xSubd;
	double areaYsize = 1.0 / ySubd;
	
	double areaXstart = column * areaXsize;
	double areaYstart = row * areaYsize;
	
	double p_x = areaXstart + areaXsize * randomFloat();
	double p_y = areaYstart + areaYsize * randomFloat();
	
	// check if shaded point is behind the lamp:
	Vector shadedPointInLightSpace = T.untransformPoint(shadePos);
	if (shadedPointInLightSpace.y > 0) {
		samplePos.makeZero();
		color.makeZero();
		return;
	}
	
	Vector pointOnLight(p_x - 0.5, 0, p_y - 0.5); // ([-0.5..0.5], 0, [-0.5..0.5])
	
	samplePos = T.transformPoint(pointOnLight);
	color = this->color * this->power;
}
