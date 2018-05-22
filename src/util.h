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
 * @File util.h
 * @Brief a few useful short functions
 */
#pragma once

#include <stdlib.h>
#include <math.h>
#include <string>
#include <vector>
#include "constants.h"

// get the count (number of elements of some array). E.g. "int a[8]; int b[11][2]; COUNT_OF(a) = 8; COUNT_OF(b) = 11"
#define COUNT_OF(someArray) (int(sizeof(someArray) / sizeof(someArray[0])))

inline double signOf(double x) { return x > 0 ? +1 : -1; }
inline double sqr(double a) { return a * a; }
inline double toRadians(double angle) { return angle / 180.0 * PI; }
inline double toDegrees(double angle_rad) { return angle_rad / PI * 180.0; }
inline int nearestInt(float x) { return (int) floor(x + 0.5f); }

std::string upCaseString(std::string s); //!< returns the string in UPPERCASE
std::string extensionUpper(const char* fileName); //!< Given a filename, return its extension in UPPERCASE
std::vector<std::string> tokenize(std::string s);
std::vector<std::string> split(std::string s, char separator);

long long getTicks(); //!< returns the current ticks, measured in ms

/// a simple RAII class for FILE* pointers.
class FileRAII {
        FILE* held;
public:
        FileRAII(FILE* init): held(init) {}
        ~FileRAII() { if (held) fclose(held); held = NULL; }
        FileRAII(const FileRAII&) = delete;
        FileRAII& operator = (const FileRAII&) = delete;
};

bool fileExists(const char* fn);
