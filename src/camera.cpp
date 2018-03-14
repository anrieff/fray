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
 * @File camera.cpp
 * @Brief Implementation of the raytracing camera.
 */

#include "camera.h"
#include "matrix.h"
#include "util.h"
#include "sdl.h"

void Camera::beginFrame()
{
	Vector C = Vector(-aspectRatio, 1, 1);
	Vector B = Vector(0, 0, 1);
	Vector BC = C - B;
	double lenBC = BC.length();
	double lenWanted = tan(toRadians(fov/2));
	double m = lenWanted / lenBC;
	topLeft    = Vector(-aspectRatio * m, +m, 1);
	topRight   = Vector(+aspectRatio * m, +m, 1);
	bottomLeft = Vector(-aspectRatio * m, -m, 1);
	w = frameWidth();
	h = frameHeight();
	
	Matrix rotation = rotationAroundZ(roll) * rotationAroundX(pitch) * rotationAroundY(yaw);
	topLeft *= rotation;
	topRight *= rotation;
	bottomLeft *= rotation;
}

Ray Camera::getScreenRay(double x, double y)
{
	Ray result;
	result.dir = topLeft + (topRight - topLeft) * (x / w)
	                     + (bottomLeft - topLeft) * (y / h);
	result.dir.normalize();
	result.start = this->pos;
	return result;
}
