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

extern Vector lightPos;
extern Color lightColor;
extern double lightIntensity;
extern Color ambientLightColor;


Color ConstantShader::shade(Ray ray, const IntersectionInfo& info)
{
	return color;
}

Color CheckerTexture::sample(Ray ray, const IntersectionInfo& info)
{
	int integerX = int(floor(info.u * scaling)); // 5.5 -> 5
	int integerY = int(floor(info.v * scaling)); // -3.2 -> -4
	
	return ((integerX + integerY) % 2 == 0) ? color1 : color2;
}

Color Lambert::shade(Ray ray, const IntersectionInfo& info)
{
	Color diffuseColor = diffuseTex->sample(ray, info);
	
	double lightDistSqr = (info.ip - lightPos).lengthSqr();
	Vector toLight = (lightPos - info.ip);
	toLight.normalize();
	
	Vector n = faceforward(ray.dir, info.norm);
	
	double cosAngle = dot(toLight, n);
	double lightMultiplier = lightIntensity * cosAngle / lightDistSqr;
	
	lightMultiplier = max(0.0, lightMultiplier);
	
	Color result = diffuseColor * ambientLightColor;
	
	if (visible(info.ip + n * 1e-6, lightPos))
		result += diffuseColor * lightColor * lightMultiplier;
	
	return result;
}

Color Phong::shade(Ray ray, const IntersectionInfo& info)
{
	Color diffuseColor = diffuseTex->sample(ray, info);
	
	double lightDistSqr = (info.ip - lightPos).lengthSqr();
	Vector toLight = (lightPos - info.ip);
	toLight.normalize();
	
	Vector n = faceforward(ray.dir, info.norm);
	
	double cosAngle = max(0.0, dot(toLight, n));
	double lightMultiplier = lightIntensity * cosAngle / lightDistSqr;
	
	Color result = diffuseColor * ambientLightColor;
	
	if (visible(info.ip + n * 1e-6, lightPos)) {
		result += diffuseColor * lightColor * lightMultiplier;
		
		Vector fromLight = -toLight;
		Vector r = reflect(fromLight, n);
		double cosCameraReflection = dot(-ray.dir, r);
		if (cosCameraReflection > 0)
			result += lightColor * specularColor
			          * pow(cosCameraReflection, exponent)
			          * specularMultiplier;
	}
	
	return result;
}


BitmapTexture::BitmapTexture(const char* filename)
{
	bmp.loadImage(filename);
}

Color BitmapTexture::sample(Ray ray, const IntersectionInfo& info)
{
	int int_x = int(floor(info.u * scaling * bmp.getWidth()));
	int int_y = int(floor(info.v * scaling * bmp.getHeight()));
	
	int_x %= bmp.getWidth();
	int_y %= bmp.getHeight();
	if (int_x < 0) int_x += bmp.getWidth();
	if (int_y < 0) int_y += bmp.getHeight();
	
	return bmp.getPixel(int_x, int_y);
}

Color Reflection::shade(Ray ray, const IntersectionInfo& info)
{
	Vector n = faceforward(ray.dir, info.norm);
	
	Ray newRay = ray;
	newRay.start = info.ip + n * 1e-6;
	newRay.dir = reflect(ray.dir, n);
	newRay.depth = ray.depth + 1;
	
	return raytrace(newRay) * mult;
}

inline float fresnel(const Vector& i, const Vector& n, float ior)
{
	// Schlick's approximation
	float f = sqr((1.0f - ior) / (1.0f + ior));
	float NdotI = (float) -dot(n, i);
	return f + (1.0f - f) * pow(1.0f - NdotI, 5.0f);
}

Color Refraction::shade(Ray ray, const IntersectionInfo& info)
{
	Vector n = faceforward(ray.dir, info.norm);
	
	
	double myIor;
	if (dot(n, info.norm) > 0) {
		// n == info.norm
		myIor = 1.0 / this->ior;
	} else {
		// n == -info.norm:
		myIor = this->ior / 1.0;
	}

	Vector refracted = refract(ray.dir, n, myIor);
	
	if (!refracted.isZero()) {
		Ray newRay = ray;
		newRay.start = info.ip - n * 1e-6;
		newRay.dir = refracted;
		newRay.depth = ray.depth + 1;
		return raytrace(newRay) * mult;
	} else {
		return Color(0, 0, 0); // total infernal refraction
	}	
}


void Layered::addLayer(Shader* shader, Color opacity, Texture* texture)
{
	if (numLayers < COUNT_OF(layers)) {
		layers[numLayers].shader = shader;
		layers[numLayers].opacity = opacity;
		layers[numLayers].texture = texture;
		numLayers++;
	}
}

Color Layered::shade(Ray ray, const IntersectionInfo& info)
{
	Color result(0, 0, 0);
	for (int i = 0; i < numLayers; i++) {
		Color opacity = (layers[i].texture ? layers[i].texture->sample(ray, info) : layers[i].opacity);
		result = layers[i].shader->shade(ray, info) * opacity + 
		         (Color(1, 1, 1) - opacity) * result;
	}
	
	return result;
}
