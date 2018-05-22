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
 * @File color.h
 * @Brief Defines the Color class, which we'll use to represent light.
 */
#pragma once

#include "util.h"

inline unsigned convertTo8bit(float x)
{
	if (x < 0) x = 0;
	if (x > 1) x = 1;
	return nearestInt(x * 255.0f);
}

/// Represents a color, using floatingpoint components in [0..1]
struct Color {
	// a union, that allows us to refer to the channels by name (::r, ::g, ::b),
	// or by index (::components[0] ...). See operator [].
	union {
		   struct { float r, g, b; };
		   float components[3];
	};

	//
	Color() {}
	Color(float _r, float _g, float _b) //!< Construct a color from floatingpoint values
	{
		setColor(_r, _g, _b);
	}
	explicit Color(unsigned rgbcolor) //!< Construct a color from R8G8B8 value like "0xffce08"
	{
		b = (rgbcolor & 0xff) / 255.0f;
		g = ((rgbcolor >> 8) & 0xff) / 255.0f;
		r = ((rgbcolor >> 16) & 0xff) / 255.0f;
	}
	/// convert to RGB32, with channel shift specifications. The default values are for
	/// the blue channel occupying the least-significant byte
	unsigned toRGB32(int redShift = 16, int greenShift = 8, int blueShift = 0)
	{
		unsigned ir = convertTo8bit(r);
		unsigned ig = convertTo8bit(g);
		unsigned ib = convertTo8bit(b);
		return (ib << blueShift) | (ig << greenShift) | (ir << redShift);
	}
	/// make black
	void makeZero(void)
	{
		r = g = b = 0;
	}
	/// set the color explicitly
	void setColor(float _r, float _g, float _b)
	{
		r = _r;
		g = _g;
		b = _b;
	}
	/// get the intensity of the color (direct)
	float intensity(void)
	{
		return (r + g + b) / 3;
	}
	/// get the perceptual intensity of the color
	float intensityPerceptual(void)
	{
		return (r * 0.299 + g * 0.587 + b * 0.114);
	}
	/// Accumulates some color to the current
	void operator += (const Color& rhs)
	{
		r += rhs.r;
		g += rhs.g;
		b += rhs.b;
	}
	/// multiplies the color
	void operator *= (float multiplier)
	{
		r *= multiplier;
		g *= multiplier;
		b *= multiplier;
	}
	/// multiplies the color with another color
	void operator *= (const Color& rhs)
	{
		r *= rhs.r;
		g *= rhs.g;
		b *= rhs.b;
	}
	/// divides the color
	void operator /= (float divider)
	{
		r /= divider;
		g /= divider;
		b /= divider;
	}	
	/// fetch r, g or b (depending on index)
	inline const float& operator[] (int index) const
	{
		return components[index];
	}
	/// fetch r, g or b (depending on index)
	inline float& operator[] (int index)
	{
		return components[index];
	}
	
	inline void adjustSaturation(float amount)
	{
		float mid = (r + g + b) / 3.0f;
		r = mid + (r - mid) * amount;
		g = mid + (g - mid) * amount;
		b = mid + (b - mid) * amount;
	}
};

/// adds two colors
inline Color operator + (const Color& a, const Color& b)
{
	return Color(a.r + b.r, a.g + b.g, a.b + b.b);
}

/// subtracts two colors
inline Color operator - (const Color& a, const Color& b)
{
	return Color(a.r - b.r, a.g - b.g, a.b - b.b);
}

/// multiplies two colors
inline Color operator * (const Color& a, const Color& b)
{
	return Color(a.r * b.r, a.g * b.g, a.b * b.b);
}

/// multiplies a color by some multiplier
inline Color operator * (const Color& a, float multiplier)
{
	return Color(a.r * multiplier, a.g * multiplier, a.b * multiplier);
}

/// multiplies a color by some multiplier
inline Color operator * (float multiplier, const Color& a)
{
	return Color(a.r * multiplier, a.g * multiplier, a.b * multiplier);
}

/// divides some color
inline Color operator / (const Color& a, float divider)
{
	return Color(a.r / divider, a.g / divider, a.b / divider);
}
