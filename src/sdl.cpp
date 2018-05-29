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
 * @File sdl.cpp
 * @Brief Implements the interface to SDL (mainly drawing to screen functions)
 */
#ifndef _WIN32
#include <SDL/SDL.h>
#else
#include <SDL.h>
#endif // _WIN32
#include <stdio.h>
#include "sdl.h"
#include "bitmap.h"

SDL_Surface* screen = NULL;
volatile bool rendering; // used in main/worker thread synchronization
SDL_Thread *render_thread;
SDL_mutex *render_lock;
bool render_async, wantToQuit = false;

/// try to create a frame window with the given dimensions
bool initGraphics(int frameWidth, int frameHeight)
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("Cannot initialize SDL: %s\n", SDL_GetError());
		return false;
	}
	screen = SDL_SetVideoMode(frameWidth, frameHeight, 32, 0);
	if (!screen) {
		printf("Cannot set video mode %dx%d - %s\n", frameWidth, frameHeight, SDL_GetError());
		return false;
	}
	return true;
}

/// closes SDL graphics
void closeGraphics(void)
{
	SDL_Quit();
}

/// displays a VFB (virtual frame buffer) to the real framebuffer, with the necessary color clipping
void displayVFB(Color vfb[VFB_MAX_SIZE][VFB_MAX_SIZE])
{
	int rs = screen->format->Rshift;
	int gs = screen->format->Gshift;
	int bs = screen->format->Bshift;
	for (int y = 0; y < screen->h; y++) {
		Uint32 *row = (Uint32*) ((Uint8*) screen->pixels + y * screen->pitch);
		for (int x = 0; x < screen->w; x++)
			row[x] = vfb[y][x].toRGB32(rs, gs, bs);
	}
	SDL_Flip(screen);
}

/// waits the user to indicate he wants to close the application (by either clicking on the "X" of the window,
/// or by pressing ESC)
void waitForUserExit(void)
{
	SDL_Event ev;
	while (1) {
		while (SDL_WaitEvent(&ev)) {
			switch (ev.type) {
				case SDL_QUIT:
					return;
				case SDL_KEYDOWN:
				{
					switch (ev.key.keysym.sym) {
						case SDLK_ESCAPE:
							return;
						default:
							break;
					}
				}
				default:
					break;
			}
		}
	}
}

/// returns the frame width
int frameWidth(void)
{
	if (screen) return screen->w;
	return 0;
}

/// returns the frame height
int frameHeight(void)
{
	if (screen) return screen->h;
	return 0;
}

// find an unused file name like 'fray_0005.bmp'
static void findUnusedFN(char fn[], const char* suffix)
{
	char temp[256];
	int idx = 0;
	while (idx < 10000) {
		sprintf(temp, "fray_%04d.bmp", idx);
		FILE* f = fopen(temp, "rb");
		if (!f) {
			sprintf(temp, "fray_%04d.exr", idx);
			f = fopen(temp, "rb");
		}
		if (!f) break; // file doesn't exist - use that
		fclose(f);
		idx++;
	}
	sprintf(fn, "fray_%04d.%s", idx, suffix); 
}

bool takeScreenshot(const char* filename)
{
	extern Color vfb[VFB_MAX_SIZE][VFB_MAX_SIZE]; // from main.cpp
	
	Bitmap bmp;
	bmp.generateEmptyImage(frameWidth(), frameHeight());
	for (int y = 0; y < frameHeight(); y++)
		for (int x = 0; x < frameWidth(); x++)
			bmp.setPixel(x, y, vfb[y][x]);
	bool res = bmp.saveImage(filename);
	if (res) printf("Saved a screenshot as `%s'\n", filename);
	else printf("Failed to take a screenshot\n");
	return res;
}

bool takeScreenshotAuto(Bitmap::OutputFormat fmt)
{
	char fn[256];
	findUnusedFN(fn, fmt == Bitmap::outputFormat_BMP ? "bmp" : "exr");
	return takeScreenshot(fn);
}

static void handleEvent(SDL_Event& ev)
{
	switch (ev.type) {
		case SDL_QUIT:
			wantToQuit = true;
			return;
		case SDL_KEYDOWN:
		{
			switch (ev.key.keysym.sym) {
				case SDLK_ESCAPE:
					wantToQuit = true;
					return;
				case SDLK_F12:
					// Shift+F12: screenshot in EXR; else, do it in (gamma-compressed) BMP.
					if (ev.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT))
						takeScreenshotAuto(Bitmap::outputFormat_EXR);
					else
						takeScreenshotAuto(Bitmap::outputFormat_BMP);
					break;
				default:
					break;
			}
			break;
		}
		case SDL_MOUSEBUTTONDOWN:
		{
			// raytrace a single ray at the given pixel
			extern void debugRayTrace(int x, int y);
			debugRayTrace(ev.button.x, ev.button.y);
			break;
		}
		default:
			break;
	}
}

class MutexRAII {
	SDL_mutex* mutex;
public:
	MutexRAII(SDL_mutex* _mutex)
	{
		mutex = _mutex;
		SDL_mutexP(mutex);
	}
	~MutexRAII()
	{
		SDL_mutexV(mutex);
	}
};

bool renderScene_threaded(void)
{
	render_async = true;
	rendering = true;
	extern int renderSceneThread(void*);
	render_thread = SDL_CreateThread(renderSceneThread, NULL);
	
	if(render_thread == NULL) { //Failed to start for some bloody reason
		rendering = render_async = false;
		return false;
	}
	
	while (!wantToQuit) {
		{
			MutexRAII raii(render_lock);
			if (!rendering) break;
			SDL_Event ev;
			while (SDL_PollEvent(&ev)) {
				handleEvent(ev);
				if (wantToQuit) break;
			}
		}
		SDL_Delay(100);
	}
	rendering = false;
	SDL_WaitThread(render_thread, NULL);
	render_thread = NULL;
	
	render_async = false;
	return true;
}
