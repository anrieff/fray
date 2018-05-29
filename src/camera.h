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
 * @File camera.h
 * @Brief Contains declaration of the raytracing camera.
 */
#pragma once

#include "vector.h"
#include "color.h"
#include "scene.h"

enum WhichCamera {
	CAMERA_CENTER,
	CAMERA_LEFT,
	CAMERA_RIGHT,
};

class Camera: public SceneElement {
	Vector topLeft, topRight, bottomLeft;
	Vector frontDir, upDir, rightDir;
	double w, h;
	double apertureSize;
public:
	
	Vector pos = Vector(0, 0, 0);
	double yaw = 0, pitch = 0, roll = 0;
	double fov = 90.0;
	double aspectRatio = 1.3333;
	double focalPlaneDist = 5.0;
	double fNumber = 2.0;
	bool dof = false;
	bool autofocus = true;
	int numDOFSamples = 32;
	double stereoSeparation = 0;
	Color leftMask = Color(1, 0, 0), rightMask = Color(0, 1, 1);
	
	void fillProperties(ParsedBlock& pb)
	{
		if (!pb.getVectorProp("position", &pos))
			pb.requiredProp("position");
		pb.getDoubleProp("aspectRatio", &aspectRatio, 1e-6);
		pb.getDoubleProp("fov", &fov, 0.0001, 179);
		pb.getDoubleProp("yaw", &yaw);
		pb.getDoubleProp("pitch", &pitch, -90, 90);
		pb.getDoubleProp("roll", &roll);
		pb.getBoolProp("dof", &dof);
		pb.getDoubleProp("fNumber", &fNumber, 0);
		pb.getIntProp("numSamples", &numDOFSamples, 1);
		pb.getDoubleProp("focalPlaneDist", &focalPlaneDist, 0.1);
		pb.getBoolProp("autofocus", &autofocus);
		pb.getDoubleProp("stereoSeparation", &stereoSeparation, 0.0);
		pb.getColorProp("leftMask", &leftMask);
		pb.getColorProp("rightMask", &rightMask);
		
		apertureSize = 4.5 / fNumber;
	}
	
	void beginFrame() override;
	
	ElementType getElementType() const { return ELEM_CAMERA; }	
	
	Ray getScreenRay(double x, double y, WhichCamera whichCamera = CAMERA_CENTER);
	Ray getDOFRay(double x, double y, WhichCamera whichCamera = CAMERA_CENTER);
	
};
