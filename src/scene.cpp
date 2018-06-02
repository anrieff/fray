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
 * @File scene.cpp
 * @Brief Scene file parsing, loading and infrastructure
 */
#include <stdio.h>
#include <ctype.h>
#include <vector>
#include <string>
#include <string.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "scene.h"
#include "matrix.h"
#include "bitmap.h"
#include "constants.h"
#include "camera.h"
#include "geometry.h"
#include "shading.h"
#include "environment.h"
#include "mesh.h"
#include "util.h"
#include "sdl.h"
#include "mesh.h"
#include "random_generator.h"
#include "heightfield.h"
#include "lights.h"
#include <assert.h>
using std::vector;
using std::string;

SceneElement::SceneElement()
{
	name[0] = 0;
}
void SceneElement::beginRender() {}
void SceneElement::beginFrame() {}
void SceneElement::fillProperties(ParsedBlock& pb) {}

class DefaultSceneParser;

class ParsedBlockImpl: public ParsedBlock {
	friend class DefaultSceneParser;
	struct LineInfo {
		int line;
		char propName[128];
		char propValue[256];
		bool recognized;

		LineInfo() {}
		LineInfo(int line, const char* name, const char* value): line(line)
		{
			strncpy(propName, name, sizeof(propName));
			strncpy(propValue, value, sizeof(propValue));
			recognized = false;
		}
	};
	std::vector<LineInfo> lines;
	int blockBegin, blockEnd; // line numbers
	SceneParser* parser;
	SceneElement* element;

	bool findProperty(const char* name, int& i_s, int& line_s, char*& value);
public:
	bool getIntProp(const char* name, int* value, int minValue = INT_MIN, int maxValue = INT_MAX);
	bool getBoolProp(const char* name, bool* value);
	bool getFloatProp(const char* name, float* value, float minValue = -LARGE_FLOAT, float maxValue = LARGE_FLOAT);
	bool getDoubleProp(const char* name, double* value, double minValue = -LARGE_DOUBLE, double maxValue = LARGE_DOUBLE);
	bool getColorProp(const char* name, Color* value, float minCompValue = -LARGE_FLOAT, float maxCompValue = LARGE_FLOAT);
	bool getVectorProp(const char* name, Vector* value);
	bool getGeometryProp(const char* name, Geometry** value);
	bool getIntersectableProp(const char* name, Intersectable** value);
	bool getShaderProp(const char* name, Shader** value);
	bool getTextureProp(const char* name, Texture** value);
	bool getNodeProp(const char* name, Node** value);
	bool getStringProp(const char* name, char* value);
	bool getFilenameProp(const char* name, char* value);
	bool getBitmapFileProp(const char* name, Bitmap& value);
	void getTransformProp(Transform& T);
	void requiredProp(const char* name);
	void signalError(const char* msg);
	void signalWarning(const char* msg);
	int getBlockLines();
	void getBlockLine(int idx, int& srcLine, char head[], char tail[]);
	SceneParser& getParser();
};



SyntaxError::SyntaxError() {
	line = -1;
	msg[0] = 0;
}

SyntaxError::SyntaxError(int line, const char* format, ...)
{
	this->line = line;
	va_list ap;
	va_start(ap, format);
#ifdef _MSC_VER
#	define vsnprintf _vsnprintf
#endif
	vsnprintf(msg, sizeof(msg), format, ap);
	va_end(ap);
}

FileNotFoundError::FileNotFoundError() {}
FileNotFoundError::FileNotFoundError(int line, const char* filename)
{
	this->line = line;
	strcpy(this->filename, filename);
}

bool ParsedBlockImpl::findProperty(const char* name, int& i_s, int& line_s, char*& value)
{
	for (int i = 0; i < (int) lines.size(); i++) if (!strcmp(lines[i].propName, name)) {
		i_s = i;
		line_s = lines[i].line;
		value = lines[i].propValue;
		lines[i].recognized = true;
		return true;
	}
	return false;
}

#define PBEGIN\
	int i_s = 0; \
	char* value_s; \
	int line = 0; \
	if (!findProperty(name, i_s, line, value_s)) return false;
bool ParsedBlockImpl::getIntProp(const char* name, int* value, int minValue, int maxValue)
{
	PBEGIN;
	int x;
	if (1 != sscanf(value_s, "%d", &x)) throw SyntaxError(line, "Invalid integer");
	if (x < minValue || x > maxValue) throw SyntaxError(line, "Value outside the allowed bounds (%d .. %d)\n", minValue, maxValue);
	*value = x;
	return true;
}

bool ParsedBlockImpl::getBoolProp(const char* name, bool* value)
{
	PBEGIN;
	if (!strcmp(value_s, "off") || !strcmp(value_s, "false") || !strcmp(value_s, "0")) *value = false;
	else *value = true;
	return true;
}

bool ParsedBlockImpl::getFloatProp(const char* name, float* value, float minValue, float maxValue)
{
	PBEGIN;
	float x;
	if (1 != sscanf(value_s, "%f", &x)) throw SyntaxError(line, "Invalid float");
	if (x < minValue || x > maxValue) throw SyntaxError(line, "Value outside the allowed bounds (%f .. %f)\n", minValue, maxValue);
	*value = x;
	return true;
}

bool ParsedBlockImpl::getDoubleProp(const char* name, double* value, double minValue, double maxValue)
{
	PBEGIN;
	double x;
	if (1 != sscanf(value_s, "%lf", &x)) throw SyntaxError(line, "Invalid double");
	if (x < minValue || x > maxValue) throw SyntaxError(line, "Value outside the allowed bounds (%f .. %f)\n", minValue, maxValue);
	*value = x;
	return true;
}

void stripBracesAndCommas(char *s)
{
	int l = (int) strlen(s);
	for (int i = 0; i < l; i++) {
		char& c = s[i];
		if (c == ',' || c == '(' || c == ')') c = ' ';
	}
}

bool ParsedBlockImpl::getColorProp(const char* name, Color* value, float minCompValue, float maxCompValue)
{
	PBEGIN;
	stripBracesAndCommas(value_s);
	Color c;
	if (3 != sscanf(value_s, "%f%f%f", &c.r, &c.g, &c.b)) throw SyntaxError(line, "Invalid color");
	if (c.r < minCompValue || c.r > maxCompValue) throw SyntaxError(line, "Color R value outside the allowed bounds (%f .. %f)\n", minCompValue, maxCompValue);
	if (c.g < minCompValue || c.g > maxCompValue) throw SyntaxError(line, "Color G value outside the allowed bounds (%f .. %f)\n", minCompValue, maxCompValue);
	if (c.b < minCompValue || c.b > maxCompValue) throw SyntaxError(line, "Color B value outside the allowed bounds (%f .. %f)\n", minCompValue, maxCompValue);
	*value = c;
	return true;
}

bool ParsedBlockImpl::getVectorProp(const char* name, Vector* value)
{
	PBEGIN;
	stripBracesAndCommas(value_s);
	Vector v;
	if (3 != sscanf(value_s, "%lf%lf%lf", &v.x, &v.y, &v.z)) throw SyntaxError(line, "Invalid vector");
	*value = v;
	return true;
}

bool ParsedBlockImpl::getGeometryProp(const char* name, Geometry** value)
{
	PBEGIN;
	Geometry* g = parser->findGeometryByName(value_s);
	if (!g) throw SyntaxError(line, "Geometry not defined");
	*value = g;
	return true;
}

bool ParsedBlockImpl::getIntersectableProp(const char* name, Intersectable** value)
{
	PBEGIN;
	Geometry* g = parser->findGeometryByName(value_s);
	if (g) {
		*value = g;
		return true;
	}
	Node* node = parser->findNodeByName(value_s);
	if (!node) throw SyntaxError(line, "Intersectable by that name not defined");
	*value = node;
	return true;
}

bool ParsedBlockImpl::getShaderProp(const char* name, Shader** value)
{
	PBEGIN;
	Shader* s = parser->findShaderByName(value_s);
	if (!s) throw SyntaxError(line, "Shader not defined");
	*value = s;
	return true;
}

bool ParsedBlockImpl:: getTextureProp(const char* name, Texture** value)
{
	PBEGIN;
	Texture* t = parser->findTextureByName(value_s);
	if (!t) throw SyntaxError(line, "Texture not defined");
	*value = t;
	return true;
}

bool ParsedBlockImpl::getNodeProp(const char* name, Node** value)
{
	PBEGIN;
	Node* n = parser->findNodeByName(value_s);
	if (!n) throw SyntaxError(line, "Node not defined");
	*value = n;
	return true;
}

bool ParsedBlockImpl::getStringProp(const char* name, char* value)
{
	PBEGIN;
	strcpy(value, value_s);
	return true;
}

bool ParsedBlockImpl::getFilenameProp(const char* name, char* value)
{
	PBEGIN;
	strcpy(value, value_s);
	if (parser->resolveFullPath(value)) return true;
	else throw FileNotFoundError(line, value_s);
}

bool ParsedBlockImpl::getBitmapFileProp(const char* name, Bitmap& bmp)
{
	PBEGIN;
	char filename[256];
	strcpy(filename, value_s);
	if (!parser->resolveFullPath(filename)) throw FileNotFoundError(line, filename);
	return bmp.loadImage(filename);
}

void ParsedBlockImpl::getTransformProp(Transform& T)
{
	for (int i = 0; i < (int) lines.size(); i++) {
		double x, y, z;
		if (!strcmp(lines[i].propName, "scale")) {
			lines[i].recognized = true;
			get3Doubles(lines[i].line, lines[i].propValue, x, y, z);
			T.scale(x, y, z);
			continue;
		}
		if (!strcmp(lines[i].propName, "rotate")) {
			lines[i].recognized = true;
			get3Doubles(lines[i].line, lines[i].propValue, x, y, z);
			T.rotate(x, y, z);
			continue;
		}
		if (!strcmp(lines[i].propName, "translate")) {
			lines[i].recognized = true;
			get3Doubles(lines[i].line, lines[i].propValue, x, y, z);
			T.translate(Vector(x, y, z));
			continue;
		}
	}
}

void ParsedBlockImpl::requiredProp(const char* name)
{
	int k1, k2;
	char* value;
	if (!findProperty(name, k1, k2, value)) {
		throw SyntaxError(blockEnd, "Required property `%s' not defined", name);
	}
}

void ParsedBlockImpl::signalError(const char* msg)
{
	throw SyntaxError(blockEnd, msg);
}

void ParsedBlockImpl::signalWarning(const char* msg)
{
	fprintf(stderr, "Warning (at line %d): %s\n", blockEnd, msg);
}


int ParsedBlockImpl::getBlockLines()
{
	return (int) lines.size();
}
void ParsedBlockImpl::getBlockLine(int idx, int& srcLine, char head[], char tail[])
{
	lines[idx].recognized = true;
	srcLine = lines[idx].line;
	strcpy(head, lines[idx].propName);
	strcpy(tail, lines[idx].propValue);
}

SceneParser& ParsedBlockImpl::getParser()
{
	return *parser;
}

class DefaultSceneParser: public SceneParser {
	char sceneRootDir[256];
	Scene* s;
	SceneElement* curObj;
	int curLine;
	void replaceRandomNumbers(int srcLine, char line[], Random& rnd);
public:
	DefaultSceneParser();
	~DefaultSceneParser();
	SceneElement* newSceneElement(const char* className);
	bool resolveFullPath(char* path);
	Shader* findShaderByName(const char* name);
	Texture* findTextureByName(const char* name);
	Geometry* findGeometryByName(const char* name);
	Node* findNodeByName(const char* name);

	bool parse(const char* filename, Scene* s);
};

DefaultSceneParser::DefaultSceneParser()
{
	curObj = NULL;
	s = NULL;
	sceneRootDir[0] = 0;
}

DefaultSceneParser::~DefaultSceneParser()
{
}

static void stripWhiteSpace(char* s)
{
	int i = (int) strlen(s) - 1;
	while (i >= 0 && isspace(s[i])) i--;
	s[++i] = 0;
	int l = i;
	i = 0;
	while (i < l && isspace(s[i])) i++;
	if (i > 0 && i < l) {
		for (int j = 0; j <= l - i; j++)
			s[j] = s[i + j];
	}
}

bool DefaultSceneParser::parse(const char* filename, Scene* ss)
{
	Random& rnd = getRandomGen(0);
	s = ss;
	curObj = NULL;
	curLine = 0;
	s->environment = NULL;
	//
	FILE* f = fopen(filename, "rt");
	if (!f) {
		fprintf(stderr, "Cannot open scene file `%s'!\n", filename);
		return false;
	}
	int i = (int) strlen(filename) - 1;
	sceneRootDir[0] = 0;
	while (i >= 0 && (filename[i] != '/' && filename[i] != '\\')) i--;
	if (i >= 0) {
		i++;
		strncpy(sceneRootDir, filename, i);
		sceneRootDir[i] = 0;
	}
	FileRAII fraii(f);
	char line[1024];
	bool commentedOut = false;
	vector<ParsedBlockImpl> parsedBlocks;
	ParsedBlockImpl* cblock = NULL;
	while (fgets(line, sizeof(line), f)) {
		curLine++;
		if (commentedOut) {
			if (line[0] == '*' && line[1] == '/') commentedOut = false;
			continue;
		}
		char *commentBegin = NULL;
		if (strstr(line, "//")) commentBegin = strstr(line, "//");
		if (strstr(line, "#")) {
			char* temp = strstr(line, "#");
			if (!commentBegin || commentBegin > temp) commentBegin = temp;
		}
		if (commentBegin) *commentBegin = 0;
		stripWhiteSpace(line);
		if (strlen(line) == 0) continue; // empty line
		if (line[0] == '#' || (line[0] == '/' && line[1] == '/')) continue; // comment
		if (line[0] == '/' && line[1] == '*') {
			commentedOut = true;
			continue;
		}
		replaceRandomNumbers(curLine, line, rnd);
		vector<string> tokens = tokenize(line);
		if (!curObj) {
			switch (tokens.size()) {
				case 1:
				{
					if (tokens[0] == "{")
						fprintf(stderr, "Excess `}' on line %d\n", curLine);
					else
						fprintf(stderr, "Unexpected token `%s' on line %d\n", tokens[0].c_str(), curLine);
					return false;
				}
				case 2:
				{
					if (tokens[1] != "{") {
						fprintf(stderr, "A singleton object definition should end with a `{' (on line %d)\n", curLine);
						return false;
					}
					curObj = newSceneElement(tokens[0].c_str());
					if (curObj) curObj->name[0] = 0;
					break;
				}
				case 3:
				{
					if (tokens[2] != "{") {
						fprintf(stderr, "A object definition should end with a `{' (on line %d)\n", curLine);
						return false;
					}
					curObj = newSceneElement(tokens[0].c_str());
					break;
				}
				default:
				{
					fprintf(stderr, "Unexpected content on line %d!\n", curLine);
					return false;
				}
			}
			if (curObj) {
				strcpy(curObj->name, tokens[1].c_str());
				parsedBlocks.push_back(ParsedBlockImpl());
				cblock = &parsedBlocks[parsedBlocks.size() - 1];
				cblock->parser = this;
				cblock->element = curObj;
				cblock->blockBegin = curLine;
			} else {
				fprintf(stderr, "Unknown object class `%s' on line %d\n", tokens[0].c_str(), curLine);
				return false;
			}
			ElementType et = curObj->getElementType();
			switch (et) {
				case ELEM_GEOMETRY: s->geometries.push_back((Geometry*)curObj); break;
				case ELEM_SHADER: s->shaders.push_back((Shader*)curObj); break;
				case ELEM_TEXTURE: s->textures.push_back((Texture*)curObj); break;
				case ELEM_NODE: s->nodes.push_back((Node*)curObj); break;
				case ELEM_ENVIRONMENT: s->environment = (Environment*) curObj; break;
				case ELEM_CAMERA: s->camera = (Camera*)curObj; break;
				case ELEM_LIGHT: s->lights.push_back((Light*) curObj); break;
				default: break;
			}
		} else {
			if (tokens.size() == 1) {
				if (tokens[0] == "}") {
					cblock->blockEnd = curLine;
					curObj = NULL;
					cblock = NULL;
				} else {
					fprintf(stderr, "Unexpected token in object definition on line %d: `%s'\n", curLine, tokens[0].c_str());
					return false;
				}
			} else {
				// try to find a property with that name...
				int i = (int) tokens[0].length();
				while (isspace(line[i])) i++;
				int l = (int) strlen(line) - 1;
				if (i < l && line[i] == '"' && line[l] == '"') { // strip the quotes of a quoted argument
					line[l] = 0;
					i++;
				}
				ParsedBlockImpl::LineInfo info;
				cblock->lines.push_back(ParsedBlockImpl::LineInfo(curLine, tokens[0].c_str(), line + i));
			}
		}
	}
	if (curObj) {
		fprintf(stderr, "Unfinished object definition at EOF!\n");
		return false;
	}
	const int element_types_order[] = {
		ELEM_SETTINGS, ELEM_CAMERA, ELEM_ENVIRONMENT, ELEM_LIGHT, ELEM_GEOMETRY, ELEM_TEXTURE, ELEM_SHADER, ELEM_NODE, //ELEM_ATMOSPHERIC
	};
	// process all parsed blocks, but first process all singletons, then process geometries first, etc.
	for (int ei = 0; ei < (int) (sizeof(element_types_order) / sizeof(element_types_order[0])); ei++) {
		for (int i = 0; i < (int) parsedBlocks.size(); i++) {
			ParsedBlockImpl& pb = parsedBlocks[i];
			if (pb.element->getElementType() == element_types_order[ei]) {
				try {
					pb.element->fillProperties(pb);
				}
				catch (SyntaxError err) {
					fprintf(stderr, "%s:%d: Syntax error on line %d: %s\n", filename, err.line, err.line, err.msg);
					return false;
				}
				catch (FileNotFoundError err) {
					fprintf(stderr, "%s:%d: Required file not found (%s) (required at line %d)\n", filename, err.line, err.filename, err.line);
					return false;
				}
				for (int i = 0; i < (int) pb.lines.size(); i++)
					if (!pb.lines[i].recognized)
						fprintf(stderr, "%s:%d: Warning: the property `%s' isn't recognized!\n", filename, pb.lines[i].line, pb.lines[i].propName);
			}
		}
	}
	// filter out the nodes[] array; any nodes, which don't have a shader attached are transferred to the
	// subnodes array:
	for (int i = (int) s->nodes.size() - 1; i >= 0; i--) {
		if (!s->nodes[i]->shader) {
			s->superNodes.push_back(s->nodes[i]);
			s->nodes.erase(s->nodes.begin() + i);
		}
	}
	return true;
}

Shader* DefaultSceneParser::findShaderByName(const char* name)
{
	for (auto& shader: s->shaders) {
		if (!strcmp(shader->name, name)) {
			return shader;
		}
	}
	return NULL;
}
Geometry* DefaultSceneParser::findGeometryByName(const char* name)
{
	for (auto& geom: s->geometries) {
		if (!strcmp(geom->name, name)) {
			return geom;
		}
	}
	return NULL;
}
Texture* DefaultSceneParser::findTextureByName(const char* name)
{
	for (auto& tex: s->textures) {
		if (!strcmp(tex->name, name)) {
			return tex;
		}
	}
	return NULL;
}
Node* DefaultSceneParser::findNodeByName(const char* name)
{
	for (auto& node: s->nodes) {
		if (!strcmp(node->name, name)) {
			return node;
		}
	}
	return NULL;
}

void DefaultSceneParser::replaceRandomNumbers(int srcLine, char s[], Random& rnd)
{
	while (strstr(s, "randfloat")) {
		char* p = strstr(s, "randfloat");
		int i, j;
		for (i = 0; p[i] && p[i] != '('; i++);
		if (p[i] != '(') throw SyntaxError(srcLine, "randfloat in inexpected format");
		for (j = i; p[j] && p[j] != ')'; j++);
		if (p[j] != ')') throw SyntaxError(srcLine, "randfloat in inexpected format");
		p[j] = 0;
		float f1, f2;
		if (2 != sscanf(p + i + 1, "%f,%f", &f1, &f2)) throw SyntaxError(srcLine, "bad randfloat format (expected: randfloat(<min>, <max>))");
		if (f1 > f2) throw SyntaxError(srcLine, "bad randfloat format (min > max)");
		float res = rnd.randfloat() * (f2 - f1) + f1;
		for (int k = 0; k <= j; k++)
			p[k] = ' ';
		char temp[30];
		sprintf(temp, "%.5f", res);
		int l = (int) strlen(temp);
		assert (l < j);
		for (int i = 0; i < l; i++)
			p[i] = temp[i];
	}
	while (strstr(s, "randint")) {
		char* p = strstr(s, "randint");
		int i, j;
		for (i = 0; p[i] && p[i] != '('; i++);
		if (p[i] != '(') throw SyntaxError(srcLine, "randint in inexpected format");
		for (j = i; p[j] && p[j] != ')'; j++);
		if (p[j] != ')') throw SyntaxError(srcLine, "randint in inexpected format");
		p[j] = 0;
		int f1, f2;
		if (2 != sscanf(p + i + 1, "%d,%d", &f1, &f2)) throw SyntaxError(srcLine, "bad randint format (expected: randint(<min>, <max>))");
		if (f1 > f2) throw SyntaxError(srcLine, "bad randint format (min > max)");
		int res = rnd.randint(f1, f2);
		for (int k = 0; k <= j; k++)
			p[k] = ' ';
		char temp[30];
		sprintf(temp, "%d", res);
		int l = (int) strlen(temp);
		assert (l < j);
		for (int i = 0; i < l; i++)
			p[i] = temp[i];
	}
}

void get3Doubles(int srcLine, char* expression, double& d1, double& d2, double& d3)
{
	int l = (int) strlen(expression);
	for (int i = 0; i < l; i++) {
		char c = expression[i];
		if (c == '(' || c == ')' || c == ',') expression[i] = ' ';
	}
	if (3 != sscanf(expression, "%lf%lf%lf", &d1, &d2, &d3)) {
		throw SyntaxError(srcLine, "Expected three double values");
	}
}

bool getFrontToken(char* s, char* frontToken)
{
	int l = (int) strlen(s);
	int i = 0;
	while (i < l && isspace(s[i])) i++;
	if (i == l) return false;
	int j = 0;
	while (i < l && !isspace(s[i])) frontToken[j++] = s[i++];
	if (i == l) return false;
	frontToken[j] = 0;
	j = 0;
	while (i <= l) s[j++] = s[i++];
	return true;
}

bool getLastToken(char* s, char* backToken)
{
	int l = (int) strlen(s);
	int i = l - 1;
	while (i >= 0 && isspace(s[i])) i--;
	if (i < 0) return false;
	int j = i;
	while (j >= 0 && !isspace(s[j])) j--;
	if (j < 0) return false;
	strncpy(backToken, &s[j + 1], i - j);
	backToken[i - j] = 0;
	s[++j] = 0;
	return true;
}

void stripPunctuation(char* s)
{
	char temp[1024];
	strncpy(temp, s, sizeof(temp));
	int l = (int) strlen(temp);
	int j = 0;
	for (int i = 0; i < l; i++) {
		if (!isspace(temp[i]) && temp[i] != ',')
			s[j++] = temp[i];
	}
	s[j] = 0;
}

bool DefaultSceneParser::resolveFullPath(char* path)
{
	char temp[256];
	strcpy(temp, sceneRootDir);
	strcat(temp, path);
	if (fileExists(temp)) {
		strcpy(path, temp);
		return true;
	} else {
		return false;
	}
}

Scene::Scene()
{
	environment = NULL;
	camera = NULL;
}

template<typename T>
void disposeArray(vector<T>& objects)
{
	for (auto& object: objects)
		if (object) delete object;
	objects.clear();
}

Scene::~Scene()
{
	disposeArray(geometries);
	disposeArray(nodes);
	disposeArray(superNodes);
	disposeArray(textures);
	disposeArray(shaders);
	disposeArray(lights);
	if (environment) delete environment;
	environment = NULL;
	if (camera) delete camera;
	camera = NULL;
}

bool Scene::parseScene(const char* filename)
{
	DefaultSceneParser parser;
	return parser.parse(filename, this);
}

void Scene::beginRender()
{
	for (auto& element: geometries) element->beginRender();
	for (auto& element: textures) element->beginRender();
	for (auto& element: shaders) element->beginRender();
	for (auto& element: superNodes) element->beginRender();
	for (auto& element: nodes) element->beginRender();
	for (auto& element: lights) element->beginRender();
	camera->beginRender();
	settings.beginRender();
	if (environment) environment->beginRender();
}

void Scene::beginFrame()
{
	for (auto& element: geometries) element->beginFrame();
	for (auto& element: textures) element->beginFrame();
	for (auto& element: shaders) element->beginFrame();
	for (auto& element: superNodes) element->beginFrame();
	for (auto& element: nodes) element->beginFrame();
	for (auto& element: lights) element->beginFrame();
	camera->beginFrame();
	settings.beginFrame();
	if (environment) environment->beginFrame();
}

GlobalSettings::GlobalSettings()
{
	frameWidth = DEFAULT_FRAME_WIDTH;
	frameHeight = DEFAULT_FRAME_HEIGHT;
	wantAA = true;
	dbg = false;
	maxTraceDepth = 4;
	ambientLight.makeZero();
	saturation = 1;
	wantPrepass = true;
	gi = false;
	numPaths = 10;
	numThreads = 0;
	interactive = fullscreen = false;
}

void GlobalSettings::fillProperties(ParsedBlock& pb)
{
	pb.getIntProp("frameWidth", &frameWidth);
	pb.getIntProp("frameHeight", &frameHeight);
	pb.getColorProp("ambientLight", &ambientLight);
	pb.getIntProp("maxTraceDepth", &maxTraceDepth);
	pb.getBoolProp("dbg", &dbg);
	pb.getBoolProp("wantAA", &wantAA);
	pb.getFloatProp("saturation", &saturation, 0, 1);
	pb.getBoolProp("wantPrepass", &wantPrepass);
	pb.getBoolProp("gi", &gi);
	pb.getIntProp("pathsPerPixel", &numPaths, 1);
	pb.getIntProp("numThreads", &numThreads);
	pb.getBoolProp("interactive", &interactive);
	pb.getBoolProp("fullscreen", &fullscreen);
}

bool GlobalSettings::needAApass()
{
	return wantAA && !interactive && !scene.camera->dof && !scene.settings.gi;
}

SceneElement* DefaultSceneParser::newSceneElement(const char* className)
{
	if (!strcmp(className, "GlobalSettings")) return &s->settings;
	if (!strcmp(className, "Plane")) return new Plane;
	if (!strcmp(className, "Sphere")) return new Sphere;
	if (!strcmp(className, "Cube")) return new Cube;
	if (!strcmp(className, "CsgPlus")) return new CsgPlus;
	if (!strcmp(className, "CsgAnd")) return new CsgIntersect;
	if (!strcmp(className, "CsgMinus")) return new CsgMinus;
	if (!strcmp(className, "Lambert")) return new Lambert;
	if (!strcmp(className, "Phong")) return new Phong;
	if (!strcmp(className, "CheckerTexture")) return new CheckerTexture;
	if (!strcmp(className, "BitmapTexture")) return new BitmapTexture;
	if (!strcmp(className, "Refl")) return new Reflection;
	if (!strcmp(className, "Refr")) return new Refraction;
	if (!strcmp(className, "Layered")) return new Layered;
	if (!strcmp(className, "Fresnel")) return new FresnelTexture;
	if (!strcmp(className, "Node")) return new Node;
	if (!strcmp(className, "CubemapEnvironment")) return new CubemapEnvironment;
	if (!strcmp(className, "Camera")) return new Camera;
	if (!strcmp(className, "Mesh")) return new Mesh;
	if (!strcmp(className, "BumpTexture")) return new BumpTexture;
	if (!strcmp(className, "Const")) return new ConstantShader;
	if (!strcmp(className, "PointLight")) return new PointLight;
	if (!strcmp(className, "RectLight")) return new RectLight;

	return NULL;
}

Scene scene;
