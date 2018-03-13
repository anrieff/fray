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
#include "constants.h"

inline double signOf(double x) { return x > 0 ? +1 : -1; }
inline double sqr(double a) { return a * a; }
inline double toRadians(double angle) { return angle / 180.0 * PI; }
inline double toDegrees(double angle_rad) { return angle_rad / PI * 180.0; }
inline int nearestInt(float x) { return (int) floor(x + 0.5f); }

/// returns a random floating-point number in [0..1).
/// This is not a very good implementation. A better method is to be employed soon.
inline float randomFloat() { return rand() / (float) RAND_MAX; }
