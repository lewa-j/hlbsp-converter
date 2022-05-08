// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
#pragma once

#include <math.h>

struct vec2i_t
{
	int x, y;
};

struct vec2_t
{
	union
	{
		struct
		{
			float x, y;
		};
		float v[2];
	};

	static vec2_t lerp(const vec2_t &v1, const vec2_t &v2, float k)
	{
		float ik = 1.0f - k;
		return{ v1.x * ik + v2.x * k, v1.y * ik + v2.y * k };
	}
};

struct vec3_t
{
	float x, y, z;

	float dot(const vec3_t &v) const
	{
		return x * v.x + y * v.y + z * v.z;
	}

	double dot_d(const vec3_t &v) const
	{
		return x * (double)v.x + y * (double)v.y + z * (double)v.z;
	}

	vec3_t cross(const vec3_t &v) const
	{
		return {
			y * v.z - z * v.y,
			z * v.x - x * v.z,
			x * v.y - y * v.x
		};
	}

	vec3_t &normalize()
	{
		float l = sqrtf(dot(*this));
		x /= l;
		y /= l;
		z /= l;
		return *this;
	}

	float dist2(const vec3_t &v) const
	{
		float dx = v.x - x;
		float dy = v.y - y;
		float dz = v.z - z;
		return (dx * dx + dy * dy + dz * dz);
	}

	vec3_t &operator *= (float s)
	{
		x *= s;
		y *= s;
		z *= s;
		return *this;
	}

	static vec3_t lerp(const vec3_t &v1, const vec3_t &v2, float k)
	{
		float ik = 1.0f - k;
		return{ v1.x * ik + v2.x * k, v1.y * ik + v2.y * k , v1.z * ik + v2.z * k };
	}
};

inline vec3_t operator+ (const vec3_t &v1, const vec3_t &v2)
{
	return { v1.x + v2.x,v1.y + v2.y, v1.z + v2.z };
}
inline vec3_t operator- (const vec3_t &v1, const vec3_t &v2)
{
	return { v1.x - v2.x,v1.y - v2.y, v1.z - v2.z };
}
