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
 * @File scene.cpp
 * @Brief Scene file parsing, loading and infrastructure
 */
#pragma once

#include <vector>
#include <limits.h>
#include "color.h"
#include "vector.h"

enum ElementType {
	ELEM_GEOMETRY,
	ELEM_SHADER,
	ELEM_NODE,
	ELEM_TEXTURE,
	ELEM_ENVIRONMENT,
	ELEM_CAMERA,
	ELEM_SETTINGS,
	ELEM_LIGHT,
};

class SceneParser;
class Geometry;
class Intersectable;
class Shader;
struct Node;
class Texture;
class Environment;
class Camera;
class Bitmap;
class Light;
struct Transform;

class ParsedBlock;

/// An abstract base class for each element of the scene (Camera, Geometries,...)
/// i.e, anything, that could be described using our scene definition language
class SceneElement {
public:
	char name[64]; //!< A name of this element (a string like "sphere01", "myCamera", etc)
	SceneElement(); //!< A constructor. It sets the name to the empty string.
	virtual ~SceneElement() {} //!< a virtual destructor
	
	virtual ElementType getElementType() const = 0; //!< Gets the element type
	
	/**
	 * @brief set all the properties of a scene element from a parsed block
	 *
	 * This is a callback, called by the sceneparser, when it has finished
	 * parsing a block of properties, related to some scene element.
	 *
	 * Consider the following (part of) scene:
	 *
	 * Sphere mySphere01 {
	 *    center (12.5, 0.1, 0.3)
	 *    radius 5.0
	 * }
	 *
	 * A class Sphere should inherit from SceneElement and implement fillProperties() in the following manner
	 *
	 * class Sphere: public SceneElement {
	 *	Vector pos;
	 *	double radius;
	 * public:
	 * 	void fillProperties(ParsedBlock& pb) {
	 *		pb.getVectorProp("center", &pos);
	 *		pb.getDoubleProp("radius", &radius);
	 *	}
	 * };
	 *
	 * In fact, one would usually want to have a constructor that initializes the two
	 * properties to some default values.
	 * Also, for the sake of consistency, one should really rename that `pos' to `center' in
	 * the class's definition.
	 *
	 * (the implementation of SceneElement::fillProperties() does nothing)
	 */
	virtual void fillProperties(ParsedBlock& pb);
	
	/**
	 * @brief a callback that gets called before the rendering commences
	 *
	 * If you need to setup some internal data structures before rendering has begun,
	 * you should place it here. This callback is executed after scene parsing, and before
	 * the rendering commences. You might actually use other SceneElement's, but the beginFrame()
	 * function is called for the different classes of SceneElement's in this specific order:
	 *
	 * 1) Lights
	 * 2) Geometries
	 * 3) Textures
	 * 4) Shaders
	 * 5) Nodes
	 * 6) Camera
	 * 7) GlobalSettings
	 *
	 * The order of calling beginFrame within the same group is undefined.
	 *
	 * All these callbacks are called by the Scene::beginRender() function.
	 */
	virtual void beginRender();
	
	/**
	 * @brief same as beginRender(), but gets called before each frame
	 *
	 * the difference between beginRender() and beginFrame() is that beginRender() is only
	 * called once, after parsing is done, whereas beginFrame is called before every frame
	 * (e.g., when rendering an animation).
	 */
	virtual void beginFrame();
	
	/**
	 * @brief gets an extended interface (e.g. a Texture class may implement BumpMapperInterface)
	 *
	 * @param id - an ID of an interface, which this class may or may not implement. If not, the class shall return
     *             nullptr; otherwise it shall return a pointer to the interface
     */
	virtual void* getInterface(int id) { return nullptr; }
	
	friend class SceneParser;
};

class ParsedBlock {
public:
	virtual ~ParsedBlock() {}
	// All these methods are intended to be called by SceneElement implementations
	// each method accepts two parameters: a name, and a value pointer.
	// Each method does one of these things:
	//  - the property with the given name is found and parsed successfully, in which
	//    case the value is filled in, and the method returns true.
	//  - The property is found and it wasn't parsed successfully, in which case
	//    a syntax error exception is raised.
	//  - The property is missing in the scene file, the value is untouched, and the method
	//    returns false (if this is an error, you can signal it with pb.requiredProp(name))
	//
	// Some properties also have min/max ranges. If they are specified and the parsed value
	// does not pass the range check, then a SyntaxError is raised.
	virtual bool getIntProp(const char* name, int* value, int minValue = INT_MIN, int maxValue = INT_MAX) = 0;
	virtual bool getBoolProp(const char* name, bool* value) = 0;
	virtual bool getFloatProp(const char* name, float* value, float minValue = -LARGE_FLOAT, float maxValue = LARGE_FLOAT) = 0;
	virtual bool getDoubleProp(const char* name, double* value, double minValue = -LARGE_DOUBLE, double maxValue = LARGE_DOUBLE) = 0;
	virtual bool getColorProp(const char* name, Color* value, float minCompValue = -LARGE_FLOAT, float maxCompValue = LARGE_FLOAT) = 0;
	virtual bool getVectorProp(const char* name, Vector* value) = 0;
	virtual bool getGeometryProp(const char* name, Geometry** value) = 0;
	virtual bool getIntersectableProp(const char* name, Intersectable** value) = 0;
	virtual bool getShaderProp(const char* name, Shader** value) = 0;
	virtual bool getTextureProp(const char* name, Texture** value) = 0;
	virtual bool getNodeProp(const char* name, Node** value) = 0;
	virtual bool getStringProp(const char* name, char* value) = 0; // the buffer should be 256 chars long
	
	// useful for scene assets like textures, mesh files, etc.
	// the value will hold the full filename to the file.
	// If the file/dir is not found, a FileNotFound exception is raised.
	virtual bool getFilenameProp(const char* name, char* value) = 0;
	
	// Does the same logic as getFilenameProp(), but also loads the bitmap
	// file from the specified file name. The given bitmap is first deleted if not NULL.
	virtual bool getBitmapFileProp(const char* name, Bitmap& value) = 0;
	
	// Gets a transform from the parsed block. Namely, it searches for all properties named
	// "scale", "rotate" and "translate" and applies them to T.
	virtual void getTransformProp(Transform& T) = 0;
	
	virtual void requiredProp(const char* name) = 0; // signal an error (missing property of the given name)
	
	virtual void signalError(const char* msg) = 0; // signal an error with a specified message
	virtual void signalWarning(const char* msg) = 0; // signal a warning with a specified message
	
	// some functions for direct parsed block access:
	virtual int getBlockLines() = 0;
	virtual void getBlockLine(int idx, int& srcLine, char head[], char tail[]) = 0;
	virtual SceneParser& getParser() = 0;
};

class SceneParser {
public:
	virtual ~SceneParser() {}
	// All these methods are intended to be called by SceneElement implementations:
	virtual Shader* findShaderByName(const char* name) = 0;
	virtual Texture* findTextureByName(const char* name) = 0;
	virtual Geometry* findGeometryByName(const char* name) = 0;
	virtual Node* findNodeByName(const char* name) = 0;
	
	
	/**
	 * resolveFullPath() tries to find a file (or folder), by appending the given path to the directory, where
	 * the scene file resides. The idea is that all external files (textures, meshes, etc.) are
	 * stored in the same dir where the scene file (*.qdmg) resides, and the paths to that external
	 * files do not mention any directories, just the file names.
	 *
	 * @param path (input-output) - Supply the given file name (as given in the scene file) here.
	 *                              If the function succeeds, this will return the full path to a file
	 *                              with that name, if it's found.
	 * @returns true on success, false on failure (file not found).
	 */
	virtual bool resolveFullPath(char* path) = 0;
};

struct SyntaxError {
	char msg[128];
	int line;
	SyntaxError();
	SyntaxError(int line, const char* format, ...);
};

struct FileNotFoundError {
	char filename[245];
	int line;
	FileNotFoundError();
	FileNotFoundError(int line, const char* filename);
};

/// Utility function: gets three doubles from a string in just the same way as the sceneparser will do it
/// (stripping any commas, parentheses, etc)
/// If there is error in parsing, a SyntaxError exception is raised.
void get3Doubles(int srcLine, char* expression, double& d1, double& d2, double& d3);

/// Splits a string (given in an expression) to a head token and a remaining. The head token is written into
/// `frontToken', and the remaining is copied back to expression
bool getFrontToken(char* expression, char* frontToken);

/// Splits a string (given in an expression) to a back token and a remaining. The back token is written into
/// `backToken', and the remaining is copied back to expression
bool getLastToken(char* expression, char* backToken);

void stripPunctuation(char* expression); //!< strips any whitespace or punctuation in front or in the back of a string (in-place)



/// This structure holds all global settings of the scene - frame size, antialiasing toggles, thresholds, etc...
struct GlobalSettings: public SceneElement {
	int frameWidth, frameHeight; //!< render window size

	// Lighting:
	Color ambientLight;          //!< ambient color
	
	// AA-related:
	bool wantAA;                 //!< Is Anti-Aliasing on?
	bool gi;                     //!< Is GI on?
	
	int maxTraceDepth;           //!< Maximum recursion depth
	
	bool dbg;                    //!< A debugging flag (if on, various raytracing-related procedures will dump debug info to stdout).
	float saturation; 
	
	bool wantPrepass;            //!< Coarse resolution pre-pass required (defaults to true)
	int numPaths;                //!< paths per pixel in path tracing
	
	int numThreads;              //!< # of threads for rendering; 0 = autodetect. 1 = single-threaded
	bool interactive;            //!< interactive render
	bool fullscreen;             //!< whether we should switch to fullscreen in interactive mode
		
	GlobalSettings();
	void fillProperties(ParsedBlock& pb);
	ElementType getElementType() const { return ELEM_SETTINGS; }

	bool needAApass();
};

struct Scene {
	std::vector<Geometry*> geometries;
	std::vector<Shader*> shaders;
	std::vector<Node*> nodes;
	std::vector<Node*> superNodes; // also Nodes, but without a shader attached; don't represent an scene object directly
	std::vector<Texture*> textures;
	std::vector<Light*> lights;
	Environment* environment;
	Camera* camera;
	GlobalSettings settings;
	
	Scene();
	~Scene();
	
	bool parseScene(const char* sceneFile); //!< Parses a scene file and loads the scene from it. Returns true on success.
	void beginRender(); //!< Notifies the scene so that a render is about to begin. It calls the beginRender() method of all scene elements
	void beginFrame(); //!< Notifies the scene so that a new frame is about to begin. It calls the beginFrame() method of all scene elements
};

extern Scene scene;

std::vector<std::string> tokenize(std::string s);
std::vector<std::string> split(std::string s, char separator);
