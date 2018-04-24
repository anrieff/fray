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

Reflection::Reflection(float multiplier, double glossiness, int glossinessSamples): mult(multiplier, multiplier, multiplier)
{
	pureReflection = (glossiness == 1.0);
	deflectionScaling = pow(10.0, 2 - 4*glossiness);
	numSamples = glossinessSamples;
}

Color Reflection::shade(Ray ray, const IntersectionInfo& info)
{
	Vector n = faceforward(ray.dir, info.norm);
	
	if (pureReflection) {	
		Ray newRay = ray;
		newRay.start = info.ip + n * 1e-6;
		newRay.dir = reflect(ray.dir, n);
		newRay.depth = ray.depth + 1;
		
		return raytrace(newRay) * mult;
	} else {
		Vector b, c;
		orthonormalSystem(n, b, c);
		
		Color sum(0, 0, 0);
		int numSamplesActual = ray.depth == 0 ? numSamples : LOW_GLOSSY_SAMPLES;
		for (int i = 0; i < numSamplesActual; i++) {
			double x, y;
			Vector reflected;
			while (1) {
				randomUnitDiscPoint(x, y);
				
				x *= deflectionScaling;
				y *= deflectionScaling;
				
				Vector newNormal = n + b * x + c * y;
				newNormal.normalize();
				
				reflected = reflect(ray.dir, newNormal);
				
				if (dot(reflected, n) > 0) break;
			}
			
			Ray newRay = ray;
			newRay.start = info.ip + n * 1e-6;
			newRay.dir = reflected;
			newRay.depth = ray.depth + 1;
			
			sum += raytrace(newRay) * mult;
		}
		
		return sum / numSamplesActual;
	}
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

Color FresnelTexture::sample(Ray ray, const IntersectionInfo& info)
{
	Vector n;
	double myIor;
	
	if (dot(ray.dir, info.norm) < 0) {
		n = info.norm;
		myIor = ior;
	} else {
		n = -info.norm;
		myIor = 1.0 / ior;
	}
		
	float f = fresnel(ray.dir, n, myIor);
	
	return Color(f, f, f);
}

BumpTexture::BumpTexture(const char* filename)
{
	Bitmap sourceTex;
	sourceTex.loadImage(filename);
	
	bumpTex.generateEmptyImage(sourceTex.getWidth(), sourceTex.getHeight());
	
	for (int y = 0; y < sourceTex.getHeight(); y++)
		for (int x = 0; x < sourceTex.getWidth(); x++) {
			int right = min(sourceTex.getWidth() - 1, x + 1);
			int bottom = min(sourceTex.getHeight() - 1, y + 1);
			float dx =   sourceTex.getPixel(x, y).intensity()
			           - sourceTex.getPixel(right, y).intensity();
			float dy =   sourceTex.getPixel(x, y).intensity()
			           - sourceTex.getPixel(x, bottom).intensity();
			bumpTex.setPixel(x, y, Color(dx, dy, 0));
		}
}

void BumpTexture::getDeflection(const IntersectionInfo& info, float& dx, float& dy)
{
	int int_x = int(floor(info.u * scaling * bumpTex.getWidth()));
	int int_y = int(floor(info.v * scaling * bumpTex.getHeight()));
	
	int_x %= bumpTex.getWidth();
	int_y %= bumpTex.getHeight();
	if (int_x < 0) int_x += bumpTex.getWidth();
	if (int_y < 0) int_y += bumpTex.getHeight();
	
	Color t = bumpTex.getPixel(int_x, int_y);
	dx = t.r * bumpIntensity;
	dy = t.g * bumpIntensity;
}
