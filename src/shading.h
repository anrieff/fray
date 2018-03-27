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
 * @File shading.h
 * @Brief Contains declarations of shader classes
 */
#pragma once

#include "color.h"
#include "geometry.h"
#include "bitmap.h"

class Texture {
public:
	virtual ~Texture() {}
	
	virtual Color sample(const IntersectionInfo& info) = 0;
};

class CheckerTexture: public Texture {
public:
	Color color1 = Color(0.7, 0.7, 0.7);
	Color color2 = Color(0.2, 0.2, 0.2);
	double scaling = 0.05;
	
	CheckerTexture() {}	
	CheckerTexture(const Color& color1, const Color& color2): color1(color1), color2(color2) {}
	Color sample(const IntersectionInfo& info) override;
};

class BitmapTexture: public Texture {
	Bitmap bmp;
public:
	double scaling = 1;	
	
	BitmapTexture(const char* filename);
	Color sample(const IntersectionInfo& info) override;
};

class Shader {
public:
	Texture* diffuseTex;
	
	virtual ~Shader() {}
	
	virtual Color shade(Ray ray, const IntersectionInfo& info) = 0;
};


class ConstantShader: public Shader {
public:
	Color color = Color(1, 0, 0);
	
	Color shade(Ray ray, const IntersectionInfo& info) override;
};

class Lambert: public Shader {
public:
	
	Lambert(Texture* texture)
	{
		diffuseTex = texture;
	}
	
	Color shade(Ray ray, const IntersectionInfo& info) override;
};

class Phong: public Shader {
public:
	float exponent = 10.0f;
	float specularMultiplier = 0.25f;
	Color specularColor = Color(0.75f, 0.75f, 0.75f);
	
	Phong(Texture* texture)
	{
		diffuseTex = texture;
	}
	
	Color shade(Ray ray, const IntersectionInfo& info) override;
};
