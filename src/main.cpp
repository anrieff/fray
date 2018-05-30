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
 * @File main.cpp
 * @Brief Raytracer main file
 */
#include <SDL/SDL.h>
#include <SDL/SDL_events.h>
#include <math.h>
#include <string.h>
#include <vector>
#include "util.h"
#include "sdl.h"
#include "color.h"
#include "vector.h"
#include "matrix.h"
#include "camera.h"
#include "geometry.h"
#include "mesh.h"
#include "scene.h"
#include "lights.h"
#include "shading.h"
#include "environment.h"
#include "random_generator.h"
#include "cxxptl-sdl.h"
using namespace std;

ThreadPool pool;
Color vfb[VFB_MAX_SIZE][VFB_MAX_SIZE];
char sceneFile[256] = "data/forest.fray";
const double offsets[5][2] = {
	{ 0, 0 }, 
	{ 0.6, 0 },
	{ 0.3, 0.3 },
	{ 0, 0.6 },
	{ 0.6, 0.6 },
};


bool visible(const Vector& a, const Vector& b)
{
	Ray ray;
	ray.dir = b - a;
	ray.start = a;
	double maxDist = distance(a, b);
	ray.dir.normalize();
	
	for (auto node: scene.nodes) {
		IntersectionInfo info;
		if (node->intersect(ray, info) && info.dist < maxDist) {
			return false;
		}
	}
	
	return true;
}

void applyBumpMapping(Node& closestNode, IntersectionInfo& info)
{
	if (!closestNode.bump) return;
	
	void* interface = closestNode.bump->getInterface(BumpMapperInterface::ID);
	if (interface) {
		static_cast<BumpMapperInterface*>(interface)->modifyNormal(info);
	}
}

Vector hemisphereSample(const IntersectionInfo& info)
{
	// we want unit resultRay (direction), such that dot(info.norm, resultRay) >= 0
	
	Random& rnd = getRandomGen();
	
	double u = rnd.randdouble();
	double v = rnd.randdouble();
	
	double theta = 2 * PI * u;
	double phi = acos(2 * v - 1);
	
	Vector dir(
		sin(phi) * cos(theta),
		cos(phi),
		sin(phi) * sin(theta)
	);
	
	// v is uniform in the unit sphere
	
	if (dot(dir, info.norm) > 0)
		return dir;
	else
		return -dir;
}

Color explicitLightSample(const Ray& ray, const IntersectionInfo& info, const Color& pathMultiplier, Shader* shader, Random& rnd)
{
	// try to end a path by explicitly sampling a light. If there are no lights, we can't do that:
	if (scene.lights.empty()) return Color(0, 0, 0);

	// choose a random light:
	int lightIdx = rnd.randint(0, scene.lights.size() - 1);
	Light* chosenLight = scene.lights[lightIdx];

	// evaluate light's solid angle as viewed from the intersection point, x:
	Vector x = info.ip;
	double solidAngle = chosenLight->solidAngle(info);

	// is light is too small or invisible?
	if (solidAngle == 0) return Color(0, 0, 0);

	// choose a random point on the light:
	int samplesInLight = chosenLight->getNumSamples();
	int randSample = rnd.randint(0, samplesInLight - 1);

	Vector pointOnLight;
	Color unused;
	chosenLight->getNthSample(randSample, x, pointOnLight, unused);

	// camera -> ... path ... -> x -> lightPos
	//                       are x and lightPos visible?
	if (!visible(x + info.norm * 1e-6, pointOnLight))
		return Color(0, 0, 0);

	// get the emitted light energy (color * power):
	Color L = chosenLight->getColor();


	// evaluate BRDF. It might be zero (e.g., pure reflection), so bail out early if that's the case
	Vector w_out = pointOnLight - x;
	w_out.normalize();
	Color brdfAtPoint = shader->eval(info, ray.dir, w_out);
	if (brdfAtPoint.intensity() == 0) return Color(0, 0, 0);

	// probability to hit this light's projection on the hemisphere
	// (conditional probability, since we're specifically aiming for this light):
	float probHitLightArea = 1.0f / solidAngle;

	// probability to pick this light out of all N lights:
	float probPickThisLight = 1.0f / scene.lights.size();

	// combined probability of this generated w_out ray:
	float chooseLightProb = probHitLightArea * probPickThisLight;

	/* Light flux (Li) */ /* BRDFs@path*/  /*last BRDF*/ /*MC probability*/
	return     L       *   pathMultiplier * brdfAtPoint / chooseLightProb;
}

Color pathtrace(const Ray& ray, Color pathMultiplier, Random& rnd)
{
	if (ray.depth > scene.settings.maxTraceDepth ||
		pathMultiplier.intensity() < 0.01 
		)
		return Color(0, 0, 0);
	
	Node* closestNode = nullptr;
	IntersectionInfo closestIntersection;
	closestIntersection.dist = 1e99;
	
	for (auto node: scene.nodes) {
		IntersectionInfo info;
		if (node->intersect(ray, info) && info.dist < closestIntersection.dist) {
			closestIntersection = info;
			closestNode = node;
		}
	}
	
	bool hitLight = false;
	Light* intersectedLight = nullptr;
	for (auto light: scene.lights) {
		IntersectionInfo info;
		if (light->intersect(ray, info) && info.dist < closestIntersection.dist) {
			hitLight = true;
			closestIntersection = info;
			intersectedLight = light;
		}
	}
	
	if (hitLight) {
		if (ray.flags & RF_DIFFUSE) {
			// forbid light contributions after a diffuse reflection
			return Color(0, 0, 0);
		} else {
			return intersectedLight->getColor() * pathMultiplier;
		}
	}
	
	if (!closestNode) {
		if (scene.environment)
			return scene.environment->getEnvironment(ray.dir) * pathMultiplier;
		else
			return Color(0, 0, 0);
	}
		
	applyBumpMapping(*closestNode, closestIntersection);
	
	Ray newRay = ray;
	newRay.depth++;
	newRay.start = closestIntersection.ip + closestIntersection.norm * 1e-6;
	Color brdfColor;
	float rayPdf;
	closestNode->shader->spawnRay(closestIntersection, ray, newRay, brdfColor, rayPdf);
	
	// ("sampling the light"):
	// try to end the current path with explicit sampling of some light
	Color contribLight = explicitLightSample(ray, closestIntersection, pathMultiplier,
											closestNode->shader, rnd);
	// ("sampling the BRDF"):
	// also try to extend the current path randomly:
	Ray w_out = ray;
	w_out.depth++;
	Color brdf;
	float pdf;
	closestNode->shader->spawnRay(closestIntersection, ray, w_out, brdf, pdf);

	if (pdf == -1) return Color(1, 0, 0); // BRDF not implemented
	if (pdf == 0) return Color(0, 0, 0);  // BRDF is zero


	Color contribGI = pathtrace(w_out, pathMultiplier * brdf / pdf, rnd);
	return contribLight + contribGI;
}

Color raytrace(const Ray& ray)
{
	if (ray.depth > scene.settings.maxTraceDepth) return Color(0, 0, 0);
	
	Node* closestNode = nullptr;
	IntersectionInfo closestIntersection;
	closestIntersection.dist = 1e99;
	
	for (auto node: scene.nodes) {
		IntersectionInfo info;
		if (node->intersect(ray, info) && info.dist < closestIntersection.dist) {
			closestIntersection = info;
			closestNode = node;
		}
	}
	
	bool hitLight = false;
	Light* intersectedLight = nullptr;
	for (auto light: scene.lights) {
		IntersectionInfo info;
		if (light->intersect(ray, info) && info.dist < closestIntersection.dist) {
			hitLight = true;
			closestIntersection = info;
			intersectedLight = light;
		}
	}
	
	if (hitLight) {
		return intersectedLight->getColor();
	}
	
	if (!closestNode) {
		if (scene.environment) return scene.environment->getEnvironment(ray.dir);
		else return Color(0, 0, 0);
	}
		
	applyBumpMapping(*closestNode, closestIntersection);
	
	return closestNode->shader->shade(ray, closestIntersection);
}

Color raytrace(double x, double y)
{
	return raytrace(scene.camera->getScreenRay(x, y));
}

Color trace(const Ray& ray, Random& rnd)
{
	if (scene.settings.gi) {
		return pathtrace(ray, Color(1, 1, 1), rnd);
	} else {
		return raytrace(ray);
	}
}

Color raytraceSinglePixel(double x, double y)
{
	Random& rnd = getRandomGen();
	const auto getDOFRay = [](double x, double y, WhichCamera whichCamera) {
		return scene.camera->getDOFRay(x, y, whichCamera);
	};
	const auto getScreenRay = [](double x, double y, WhichCamera whichCamera) {
		return scene.camera->getScreenRay(x, y, whichCamera);
	};
	const auto getRay = scene.camera->dof
		? std::function<Ray(double, double, WhichCamera)>(getDOFRay)
		: std::function<Ray(double, double, WhichCamera)>(getScreenRay);

	if (scene.camera->stereoSeparation > 0) {
		Ray leftRay = getRay(x, y, CAMERA_LEFT);
		Ray rightRay= getRay(x, y, CAMERA_RIGHT);
		Color colorLeft = trace(leftRay, rnd);
		Color colorRight = trace(rightRay, rnd);
		if (scene.settings.saturation != 1) {
			colorLeft.adjustSaturation(scene.settings.saturation);
			colorRight.adjustSaturation(scene.settings.saturation);

		}
		return  colorLeft * scene.camera->leftMask
		      + colorRight* scene.camera->rightMask;
	} else {
		const Ray& ray = getRay(x, y, CAMERA_CENTER);
		return trace(ray, rnd);
	}
}

class RendMT: public Parallel {
	InterlockedInt cursor;
	vector<Rect> buckets;
	int samplesPerPixel;
	Mutex mtx;
public:
	RendMT(const vector<Rect>& buckets, int samplesPerPixel): 
		cursor(0), buckets(buckets), samplesPerPixel(samplesPerPixel) {}
	void entry(int threadIdx, int threadCount) override
	{
		Random rnd = getRandomGen();
		while (1) {
			int buckId = (cursor++);
			if (buckId >= int(buckets.size())) return;
			Rect& r = buckets[buckId];
			bool ok = true;
			if (!scene.settings.interactive) {
				mtx.enter();
				ok = markRegion(r);
				mtx.leave();
				if (!ok) return;
			}
			for (int y = r.y0; y < r.y1; y++) {
				for (int x = r.x0; x < r.x1; x++) {
					Color avg(0, 0, 0);
					for (int i = 0; i < samplesPerPixel; i++) {
						Ray ray;
						float offsetX, offsetY;
						if (scene.camera->dof || scene.settings.gi) {
							offsetX = rnd.randfloat();
							offsetY = rnd.randfloat();
						} else {
							offsetX = offsets[i][0];
							offsetY = offsets[i][1];
						}
						avg += raytraceSinglePixel(x + offsetX, y + offsetY);
					}
					vfb[y][x] = avg / samplesPerPixel;
				}
			}
			if (!scene.settings.interactive) {
				mtx.enter();
				ok = displayVFBRect(r, vfb);
				mtx.leave();
				if (!ok) return;
			}
		}
	}
};

void render()
{
	scene.beginFrame();
	const int SQUARE_SIZE = 16;
	if (scene.settings.wantPrepass && !scene.settings.interactive) {
		for (int y = 0; y < frameHeight(); y += SQUARE_SIZE) {
			int ey = min(frameHeight(), y + SQUARE_SIZE);
			int cy = (y + ey) / 2;
			for (int x = 0; x < frameWidth(); x += SQUARE_SIZE) {
				int ex = min(frameWidth(), x + SQUARE_SIZE);
				int cx = (x + ex) / 2;
				Color c = raytraceSinglePixel(cx, cy);
				if (!drawRect(Rect(x, y, ex, ey), c))
					return;
			}
		}
				
	}
	
	vector<Rect> buckets = getBucketsList();
	
	int samplesPerPixel = COUNT_OF(offsets);
	if (!scene.settings.wantAA) samplesPerPixel = 1;
	if (scene.camera->dof)
		samplesPerPixel = max(samplesPerPixel, scene.camera->numDOFSamples);
	if (scene.settings.gi)
		samplesPerPixel = max(samplesPerPixel, scene.settings.numPaths);

	RendMT worker(buckets, samplesPerPixel);
	
	pool.run(&worker, scene.settings.numThreads);
}

int renderSceneThread(void* /*unused*/)
{
	render();
	rendering = false;
	return 0;
}

bool parseCmdLine(int argc, char** argv)
{
	if (argc == 1) return true;
	if (argc != 2 || !fileExists(argv[1])) {
		printf("Usage: fray [scene.fray]\n");
		return false;
	} else {
		strcpy(sceneFile, argv[1]);
		return true;
	}
}

void debugRayTrace(int x, int y)
{
	// trace a test ("debugging") ray through a clicked pixel on the screen
	Ray ray = scene.camera->getScreenRay(x, y);
	ray.flags |= RF_DEBUG;
	if (scene.settings.gi)
		pathtrace(ray, Color(1, 1, 1), getRandomGen());
	else
		raytrace(ray);
}

void mainloop(void)
{
	SDL_ShowCursor(0);
	bool running = true;
	Camera& cam = *scene.camera;
	const double MOVEMENT_PER_SEC = 20;
	const double ROTATION_PER_SEC = 50;
	const double SENSITIVITY = 0.1;

	while (running) {
		Uint32 ticksSaved = SDL_GetTicks();
		render();
		displayVFB(vfb);
		// timeDelta is how much time the frame took to render:
		double timeDelta = (SDL_GetTicks() - ticksSaved) / 1000.0;
		//
		SDL_Event ev;

		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
				case SDL_QUIT:
					running = false;
					break;
				case SDL_KEYDOWN:
				{
					switch (ev.key.keysym.sym) {
						case SDLK_ESCAPE:
							running = false;
							break;
						default:
							break;
					}
					break;
				}
			}
		}

		Uint8* keystate = SDL_GetKeyState(NULL);
		double movement = MOVEMENT_PER_SEC * timeDelta;
		double rotation = ROTATION_PER_SEC * timeDelta;
		if (keystate[SDLK_UP]) cam.move(0, +movement);
		if (keystate[SDLK_DOWN]) cam.move(0, -movement);
		if (keystate[SDLK_LEFT]) cam.move(-movement, 0);
		if (keystate[SDLK_RIGHT]) cam.move(+movement, 0);

		if (keystate[SDLK_KP8]) cam.rotate(0, +rotation);
		if (keystate[SDLK_KP2]) cam.rotate(0, -rotation);
		if (keystate[SDLK_KP4]) cam.rotate(+rotation, 0);
		if (keystate[SDLK_KP6]) cam.rotate(-rotation, 0);

		int deltax, deltay;
		SDL_GetRelativeMouseState(&deltax, &deltay);
		cam.rotate(-SENSITIVITY * deltax, -SENSITIVITY*deltay);
	}
}


int main(int argc, char** argv)
{
	if (!parseCmdLine(argc, argv)) return -1;
	initRandom(42);
	scene.parseScene(sceneFile);
	initGraphics(scene.settings.frameWidth, scene.settings.frameHeight, 
				scene.settings.fullscreen);
	
	if (scene.settings.numThreads == 0)
		scene.settings.numThreads = get_processor_count();
	
	scene.beginRender();
	if (!scene.settings.interactive) {
		setWindowCaption("fray: rendering...");
		Uint32 startTicks = getTicks();
		renderScene_threaded();
		Uint32 elapsedMs = getTicks() - startTicks;
		printf("Render took %.2fs\n", elapsedMs / 1000.0f);
		setWindowCaption("fray: rendered in %.2fs\n", elapsedMs / 1000.0f);
		displayVFB(vfb);
		if (!wantToQuit) waitForUserExit();
	} else {
		mainloop();
	}
	closeGraphics();
	printf("Exited cleanly\n");
	return 0;
}
