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
 * @File shading.h
 * @Brief Contains declarations of shader classes
 */
#pragma once

#include "color.h"
#include "geometry.h"
#include "bitmap.h"
#include "scene.h"


class Texture: public SceneElement {
public:
	virtual ~Texture() {}
	
	ElementType getElementType() const { return ELEM_TEXTURE; }
	
	virtual Color sample(const Ray& ray, const IntersectionInfo& info) = 0;
};

class CheckerTexture: public Texture {
public:
	Color color1 = Color(0.7, 0.7, 0.7);
	Color color2 = Color(0.2, 0.2, 0.2);
	double scaling = 1;

	void fillProperties(ParsedBlock& pb)
	{
		pb.getColorProp("color1", &color1);
		pb.getColorProp("color2", &color2);
		pb.getDoubleProp("scaling", &scaling);
	}
		
	CheckerTexture() {}	
	CheckerTexture(const Color& color1, const Color& color2): color1(color1), color2(color2) {}
	Color sample(const Ray& ray, const IntersectionInfo& info) override;
};

class BitmapTexture: public Texture {
	Bitmap bmp;
public:
	double scaling = 1;
	void fillProperties(ParsedBlock& pb)
	{
		pb.getDoubleProp("scaling", &scaling);
		scaling = 1/scaling;
		if (!pb.getBitmapFileProp("file", bmp))
			pb.requiredProp("file");
	}
	
	Color sample(const Ray& ray, const IntersectionInfo& info) override;
};

struct BumpMapperInterface {
	static const int ID = 0x20180522;

	virtual ~BumpMapperInterface() {}
	virtual void getDeflection(const IntersectionInfo& info, float& dx, float& dy) = 0;
	virtual void modifyNormal(IntersectionInfo& info) = 0;
};

class BumpTexture: public Texture, public BumpMapperInterface {
	Bitmap bumpTex;
public:
	double scaling = 1;
	double bumpIntensity = 10.0f;

	void fillProperties(ParsedBlock& pb)
	{
		pb.getDoubleProp("strength", &bumpIntensity);
		pb.getDoubleProp("scaling", &scaling);
		if (!pb.getBitmapFileProp("file", bumpTex))
			pb.requiredProp("file");
	}

	void* getInterface(int id)
	{
		if (id == BumpMapperInterface::ID) return (BumpMapperInterface*) this;
		return nullptr;
	}
	Color sample(const Ray& ray, const IntersectionInfo& info);
	void getDeflection(const IntersectionInfo& info, float& dx, float& dy) override;
	void modifyNormal(IntersectionInfo& info) override;
	
	void beginRender() override;
};

class BRDF {
public:
	virtual Color eval(const IntersectionInfo& x, const Vector& w_in, const Vector& w_out) = 0; 
	virtual void spawnRay(const IntersectionInfo& x, const Ray& w_in, Ray& w_out, Color& brdfColor, float& pdf) = 0; 
};

class Shader: public SceneElement, public BRDF {
public:
	Texture* diffuseTex = nullptr;
		
	virtual ~Shader() {}
	ElementType getElementType() const { return ELEM_SHADER; }
		
	virtual Color shade(const Ray& ray, const IntersectionInfo& info) = 0;
	
	Color eval(const IntersectionInfo& x, const Vector& w_in, const Vector& w_out) override
	{
		return Color(1, 0, 0);
	}
	void spawnRay(const IntersectionInfo& x, const Ray& w_in, Ray& w_out, Color& brdfColor, float& pdf) override
	{
		w_out = w_in;
		w_out.depth++;
		brdfColor = Color(1, 0, 0);
		pdf = 1;
	}
};



class ConstantShader: public Shader {
public:
	Color color = Color(1, 0, 0);
	
	Color shade(const Ray& ray, const IntersectionInfo& info) override;
};

class Lambert: public Shader {
public:
	Color color = Color(1, 1, 1);
	
	void fillProperties(ParsedBlock& pb)
	{
		pb.getColorProp("color", &color);
		pb.getTextureProp("texture", &diffuseTex);
	}

	Color shade(const Ray& ray, const IntersectionInfo& info) override;
	Color eval(const IntersectionInfo& x, const Vector& w_in, const Vector& w_out) override;
	void spawnRay(const IntersectionInfo& x, const Ray& w_in, Ray& w_out, Color& brdfColor, float& pdf) override;
};

class Phong: public Shader {
public:
	Color color = Color(1, 1, 1);
	double exponent = 10.0f;
	double specularMultiplier = 0.25f;
	Color specularColor = Color(0.75f, 0.75f, 0.75f);
	
	void fillProperties(ParsedBlock& pb)
	{
		pb.getColorProp("color", &color);
		pb.getTextureProp("texture", &diffuseTex);
		pb.getDoubleProp("specularExponent", &exponent);
		pb.getDoubleProp("specularMultiplier", &specularMultiplier);
		pb.getColorProp("specularColor", &specularColor);
	}
	
	Color shade(const Ray& ray, const IntersectionInfo& info) override;
};

class Reflection: public Shader {
	double deflectionScaling = 0;
	double glossiness = 1.0;
	bool pureReflection = false;
	int numSamples = 10;
public:
	Color mult = Color(1, 1, 1); // typically Color(0.98, 0.98, 0.98)
	
	void fillProperties(ParsedBlock& pb)
	{
		double m = 1;
		pb.getDoubleProp("multiplier", &m);
		mult = Color(m, m, m);
		pb.getDoubleProp("glossiness", &glossiness, 0, 1);
		pb.getIntProp("numSamples", &numSamples, 1);
	}
	
	void beginFrame() override
	{
		pureReflection = (glossiness == 1.0);
		deflectionScaling = pow(10.0, 2 - 4*glossiness);		
	}
	
	Color shade(const Ray& ray, const IntersectionInfo& info) override;
	Color eval(const IntersectionInfo& x, const Vector& w_in, const Vector& w_out) override;
	void spawnRay(const IntersectionInfo& x, const Ray& w_in, Ray& w_out, Color& brdfColor, float& pdf) override;
};

class Refraction: public Shader {
public:
	double ior = 1;
	Color mult = Color(1, 1, 1);
	
	void fillProperties(ParsedBlock& pb)
	{
		double m = 1;
		pb.getDoubleProp("multiplier", &m);
		mult = Color(m, m, m);
		pb.getDoubleProp("ior", &ior, 1e-6, 10);
	}
	
				
	Color shade(const Ray& ray, const IntersectionInfo& info) override;
	Color eval(const IntersectionInfo& x, const Vector& w_in, const Vector& w_out) override;
	void spawnRay(const IntersectionInfo& x, const Ray& w_in, Ray& w_out, Color& brdfColor, float& pdf) override;
};

class FresnelTexture: public Texture {
public:
	double ior = 1;
	
	void fillProperties(ParsedBlock& pb)
	{
		pb.getDoubleProp("ior", &ior, 1e-6, 10);
	}

	Color sample(const Ray& ray, const IntersectionInfo& info) override;	
};

class Layered: public Shader {
	struct Layer {
		Shader* shader;
		Color opacity;
		Texture* texture;
	};
	int numLayers = 0;
	Layer layers[32];
public:
	
	void fillProperties(ParsedBlock& pb);
	// adds another layer to the top of the layer stack (the first added layer (#0) is the most bottom one;
	// #1 is directly above it, and so forth:
	void addLayer(Shader* shader, Color opacity = Color(1, 1, 1), Texture* texture = nullptr);
	
	Color shade(const Ray& ray, const IntersectionInfo& info) override;
};
