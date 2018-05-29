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
 * @File sdl.h
 * @Brief Contains the interface to SDL
 */
#pragma once

#include "color.h"
#include "constants.h"

extern volatile bool rendering; // used in main/worker thread synchronization
extern bool wantToQuit;

bool initGraphics(int frameWidth, int frameHeight);
void closeGraphics(void);
void displayVFB(Color vfb[VFB_MAX_SIZE][VFB_MAX_SIZE]); //!< displays the VFB (Virtual framebuffer) to the real one.
void waitForUserExit(void); //!< Pause. Wait until the user closes the application
int frameWidth(void); //!< returns the frame width (pixels)
int frameHeight(void); //!< returns the frame height (pixels)
/// sets the caption of the display window. If renderTime >= 0, the
/// msg is interpreted as a format string, and must contain '%f'
void setWindowCaption(const char* msg, float renderTime = -1.0f);
bool renderScene_threaded();

struct Rect {
	int x0, y0, x1, y1, w, h;
	Rect() {}
	Rect(int _x0, int _y0, int _x1, int _y1)
	{
		x0 = _x0, y0 = _y0, x1 = _x1, y1 = _y1;
		h = y1 - y0;
		w = x1 - x0;
	}
	void clip(int maxX, int maxY); // clips the rectangle against image size
};

// generate a list of buckets (image sub-rectangles) to be rendered, in a zigzag pattern
std::vector<Rect> getBucketsList(void);

// fills a rectangle on the screen with a solid color
// fails if the render thread is about to be killed
bool drawRect(Rect r, const Color& c);

// same as displayVFB, but only updates a specific region.
// fails if the thread has to be killed
bool displayVFBRect(Rect r, Color vfb[VFB_MAX_SIZE][VFB_MAX_SIZE]);

// marks a region (places four temporary green corners)
// fails if the thread is to be killed
bool markRegion(Rect r, const Color& bracketColor = Color(0.0f, 0.0f, 0.5f));
