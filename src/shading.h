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

class Shader {
public:
	virtual ~Shader() {}
	
	virtual Color shade(Ray ray, const IntersectionInfo& info) = 0;
};

class ConstantShader: public Shader {
public:
	Color color = Color(1, 0, 0);
	
	Color shade(Ray ray, const IntersectionInfo& info) override;
};

class CheckerShader: public Shader {
public:
	Color color1 = Color(0.7, 0.7, 0.7);
	Color color2 = Color(0.2, 0.2, 0.2);
	double scaling = 0.05;
	
	CheckerShader() {}
	CheckerShader(const Color& color1, const Color& color2): color1(color1), color2(color2) {}
	
	Color shade(Ray ray, const IntersectionInfo& info) override;
};
