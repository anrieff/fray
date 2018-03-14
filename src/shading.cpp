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
 * @File shading.cpp
 * @Brief Contains implementations of shader classes
 */

#include "shading.h"
#include "main.h"
#include <algorithm>
using namespace std;

Vector lightPos (100, 200, 80);
Color lightColor(1, 1, 0.9);
double lightIntensity = 50000;
Color ambientLightColor = Color(1, 1, 1) * 0.1;

Color ConstantShader::shade(Ray ray, const IntersectionInfo& info)
{
	return color;
}

Color CheckerShader::shade(Ray ray, const IntersectionInfo& info)
{
	int integerX = int(floor(info.u * scaling)); // 5.5 -> 5
	int integerY = int(floor(info.v * scaling)); // -3.2 -> -4
	
	Color diffuseColor = ((integerX + integerY) % 2 == 0) ? color1 : color2;
	
	double lightDistSqr = (info.ip - lightPos).lengthSqr();
	Vector toLight = (lightPos - info.ip);
	toLight.normalize();
	
	double cosAngle = dot(toLight, info.norm);
	double lightMultiplier = lightIntensity * cosAngle / lightDistSqr;
	
	lightMultiplier = max(0.0, lightMultiplier);
	
	Color result = diffuseColor * ambientLightColor;
	
	if (visible(info.ip + info.norm * 1e-6, lightPos))
		result += diffuseColor * lightColor * lightMultiplier;
	
	return result;
}
