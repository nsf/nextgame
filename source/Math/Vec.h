#pragma once

#include <cmath>
#include <cstdlib>
#include "Math/Utils.h"

//------------------------------------------------------------------------------
// Vec2
//------------------------------------------------------------------------------

struct Vec2 {
	union {
		struct {
			float x, y;
		};
		float data[2];
	};

	Vec2() = default;
	Vec2(float ax, float ay): x(ax), y(ay) {}
	explicit Vec2(float v): x(v), y(v) {}

	Vec2 &operator+=(const Vec2 &r) { x+=r.x; y+=r.y; return *this; }
	Vec2 &operator-=(const Vec2 &r) { x-=r.x; y-=r.y; return *this; }
	Vec2 &operator*=(const Vec2 &r) { x*=r.x; y*=r.y; return *this; }
	Vec2 &operator/=(const Vec2 &r) { x/=r.x; y/=r.y; return *this; }

	float &operator[](int i) { return data[i]; }
	float operator[](int i) const { return data[i]; }
};

// Helper constructors
static inline Vec2 Vec2_X(float v = 1.0f) { return {v, 0}; }
static inline Vec2 Vec2_Y(float v = 1.0f) { return {0, v}; }

// Comparison operators
static inline bool operator==(const Vec2 &l, const Vec2 &r) { return l.x == r.x && l.y == r.y; }
static inline bool operator!=(const Vec2 &l, const Vec2 &r) { return l.x != r.x || l.y != r.y; }
static inline bool operator<(const Vec2 &l, const Vec2 &r)  { return l.x < r.x && l.y < r.y; }
static inline bool operator>(const Vec2 &l, const Vec2 &r)  { return l.x > r.x && l.y > r.y; }
static inline bool operator<=(const Vec2 &l, const Vec2 &r) { return l.x <= r.x && l.y <= r.y; }
static inline bool operator>=(const Vec2 &l, const Vec2 &r) { return l.x >= r.x && l.y >= r.y; }

// Unary operators
static inline Vec2 operator-(const Vec2 &v) { return {-v.x, -v.y}; }

// Binary operators (Vec2 vs Vec2)
static inline Vec2 operator+(const Vec2 &l, const Vec2 &r) { return {l.x + r.x, l.y + r.y}; }
static inline Vec2 operator-(const Vec2 &l, const Vec2 &r) { return {l.x - r.x, l.y - r.y}; }
static inline Vec2 operator*(const Vec2 &l, const Vec2 &r) { return {l.x * r.x, l.y * r.y}; }
static inline Vec2 operator/(const Vec2 &l, const Vec2 &r) { return {l.x / r.x, l.y / r.y}; }

// Functions
static inline float length2(const Vec2 &v) { return v.x*v.x + v.y*v.y; }
static inline float length(const Vec2 &v) { return sqrtf(length2(v)); }
static inline Vec2 normalize(const Vec2 &v) { return v / Vec2(length(v)); }
static inline float dot(const Vec2 &v1, const Vec2 &v2) { return v1.x*v2.x + v1.y*v2.y; }
static inline float distance2(const Vec2 &v1, const Vec2 &v2) { return length2(v2-v1); }
static inline float distance(const Vec2 &v1, const Vec2 &v2) { return length(v2-v1); }
static inline Vec2 min(const Vec2 &v1, const Vec2 &v2) { return {min(v1.x, v2.x), min(v1.y, v2.y)}; }
static inline Vec2 max(const Vec2 &v1, const Vec2 &v2) { return {max(v1.x, v2.x), max(v1.y, v2.y)}; }

//------------------------------------------------------------------------------
// Vec2i
//------------------------------------------------------------------------------

struct Vec2i {
	union {
		struct {
			int x, y;
		};
		int data[2];
	};

	Vec2i() = default;
	Vec2i(int ax, int ay): x(ax), y(ay) {}
	explicit Vec2i(int v): x(v), y(v) {}

	Vec2i &operator+=(const Vec2i &r) { x+=r.x; y+=r.y; return *this; }
	Vec2i &operator-=(const Vec2i &r) { x-=r.x; y-=r.y; return *this; }
	Vec2i &operator*=(const Vec2i &r) { x*=r.x; y*=r.y; return *this; }
	Vec2i &operator/=(const Vec2i &r) { x/=r.x; y/=r.y; return *this; }
	Vec2i &operator^=(const Vec2i &r) { x^=r.x; y^=r.y; return *this; }

	int &operator[](int i) { return data[i]; }
	int operator[](int i) const { return data[i]; }
};

// Helper constructors
static inline Vec2i Vec2i_X(int v = 1) { return {v, 0}; }
static inline Vec2i Vec2i_Y(int v = 1) { return {0, v}; }

// Comparison operators
static inline bool operator==(const Vec2i &l, const Vec2i &r) { return l.x == r.x && l.y == r.y; }
static inline bool operator!=(const Vec2i &l, const Vec2i &r) { return l.x != r.x || l.y != r.y; }
static inline bool operator<(const Vec2i &l, const Vec2i &r)  { return l.x < r.x && l.y < r.y; }
static inline bool operator>(const Vec2i &l, const Vec2i &r)  { return l.x > r.x && l.y > r.y; }
static inline bool operator<=(const Vec2i &l, const Vec2i &r) { return l.x <= r.x && l.y <= r.y; }
static inline bool operator>=(const Vec2i &l, const Vec2i &r) { return l.x >= r.x && l.y >= r.y; }

// Unary operators
static inline Vec2i operator-(const Vec2i &v) { return {-v.x, -v.y}; }

// Binary operators (Vec2i vs Vec2i)
static inline Vec2i operator+(const Vec2i &l, const Vec2i &r) { return {l.x + r.x, l.y + r.y}; }
static inline Vec2i operator-(const Vec2i &l, const Vec2i &r) { return {l.x - r.x, l.y - r.y}; }
static inline Vec2i operator*(const Vec2i &l, const Vec2i &r) { return {l.x * r.x, l.y * r.y}; }
static inline Vec2i operator/(const Vec2i &l, const Vec2i &r) { return {l.x / r.x, l.y / r.y}; }
static inline Vec2i operator^(const Vec2i &l, const Vec2i &r) { return {l.x ^ r.x, l.y ^ r.y}; }

// Functions
static inline int area(const Vec2i &v) { return v.x * v.y; }
static inline int length2(const Vec2i &v) { return v.x*v.x + v.y*v.y; }
static inline int dot(const Vec2i &v1, const Vec2i &v2) { return v1.x*v2.x + v1.y*v2.y; }
static inline int distance2(const Vec2i &v1, const Vec2i &v2) { return length2(v2-v1); }
static inline Vec2i min(const Vec2i &v1, const Vec2i &v2) { return {min(v1.x, v2.x), min(v1.y, v2.y)}; }
static inline Vec2i max(const Vec2i &v1, const Vec2i &v2) { return {max(v1.x, v2.x), max(v1.y, v2.y)}; }
static inline Vec2i floor_div(const Vec2i &a, const Vec2i &b) { return {floor_div(a.x, b.x), floor_div(a.y, b.y)}; }

static inline Vec2 ToVec2(const Vec2i &v) { return Vec2(v.x, v.y); }
static inline Vec2i ToVec2i(const Vec2 &v) { return Vec2i(v.x, v.y); }

//------------------------------------------------------------------------------
// Vec3
//------------------------------------------------------------------------------

struct Vec3 {
	union {
		struct {
			float x, y, z;
		};
		float data[3];
	};

	Vec3() = default;
	Vec3(float ax, float ay, float az): x(ax), y(ay), z(az) {}
	explicit Vec3(float v): x(v), y(v), z(v) {}

	Vec3 &operator+=(const Vec3 &r) { x+=r.x; y+=r.y; z+=r.z; return *this; }
	Vec3 &operator-=(const Vec3 &r) { x-=r.x; y-=r.y; z-=r.z; return *this; }
	Vec3 &operator*=(const Vec3 &r) { x*=r.x; y*=r.y; z*=r.z; return *this; }
	Vec3 &operator/=(const Vec3 &r) { x/=r.x; y/=r.y; z/=r.z; return *this; }

	float &operator[](int i) { return data[i]; }
	float operator[](int i) const { return data[i]; }

	Vec2 XY() const { return {x, y}; }
	Vec2 XZ() const { return {x, z}; }
	Vec2 YZ() const { return {y, z}; }
};

// Helper constructors
static inline Vec3 Vec3_X(float v = 1.0f) { return {v, 0, 0}; }
static inline Vec3 Vec3_Y(float v = 1.0f) { return {0, v, 0}; }
static inline Vec3 Vec3_Z(float v = 1.0f) { return {0, 0, v}; }

// Comparison operators
static inline bool operator==(const Vec3 &l, const Vec3 &r) { return l.x == r.x && l.y == r.y && l.z == r.z; }
static inline bool operator!=(const Vec3 &l, const Vec3 &r) { return l.x != r.x || l.y != r.y || l.z != r.z; }
static inline bool operator<(const Vec3 &l, const Vec3 &r)  { return l.x < r.x && l.y < r.y && l.z < r.z; }
static inline bool operator>(const Vec3 &l, const Vec3 &r)  { return l.x > r.x && l.y > r.y && l.z > r.z; }
static inline bool operator<=(const Vec3 &l, const Vec3 &r) { return l.x <= r.x && l.y <= r.y && l.z <= r.z; }
static inline bool operator>=(const Vec3 &l, const Vec3 &r) { return l.x >= r.x && l.y >= r.y && l.z >= r.z; }

// Unary operators
static inline Vec3 operator-(const Vec3 &v) { return {-v.x, -v.y, -v.z}; }

// Binary operators (Vec3 vs Vec3)
static inline Vec3 operator+(const Vec3 &l, const Vec3 &r) { return {l.x + r.x, l.y + r.y, l.z + r.z}; }
static inline Vec3 operator-(const Vec3 &l, const Vec3 &r) { return {l.x - r.x, l.y - r.y, l.z - r.z}; }
static inline Vec3 operator*(const Vec3 &l, const Vec3 &r) { return {l.x * r.x, l.y * r.y, l.z * r.z}; }
static inline Vec3 operator/(const Vec3 &l, const Vec3 &r) { return {l.x / r.x, l.y / r.y, l.z / r.z}; }

// Functions
static inline bool is_nan(const Vec3 &v) { return std::isnan(v.x) || std::isnan(v.y) || std::isnan(v.z); }
static inline float length2(const Vec3 &v) { return v.x*v.x + v.y*v.y + v.z*v.z; }
static inline float length(const Vec3 &v) { return sqrtf(length2(v)); }
static inline Vec3 normalize(const Vec3 &v) { return v / Vec3(length(v)); }
static inline Vec3 abs(const Vec3 &v) { return {fabsf(v.x), fabsf(v.y), fabsf(v.z)}; }
static inline float dot(const Vec3 &v1, const Vec3 &v2) { return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z; }
static inline float volume(const Vec3 &v) { return v.x * v.y * v.z; }
static inline Vec3 cross(const Vec3 &v1, const Vec3 &v2) { return {v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x}; }
static inline float distance2(const Vec3 &v1, const Vec3 &v2) { return length2(v2-v1); }
static inline float distance(const Vec3 &v1, const Vec3 &v2) { return length(v2-v1); }
static inline Vec3 min(const Vec3 &v1, const Vec3 &v2) { return {min(v1.x, v2.x), min(v1.y, v2.y), min(v1.z, v2.z)}; }
static inline Vec3 max(const Vec3 &v1, const Vec3 &v2) { return {max(v1.x, v2.x), max(v1.y, v2.y), max(v1.z, v2.z)}; }
static inline Vec3 lerp(const Vec3 &a, const Vec3 &b, float v) { return a * Vec3(1 - v) + b * Vec3(v); }
static inline Vec3 mod(const Vec3 &a, const Vec3 &b) { return Vec3(fmodf(a.x, b.x), fmodf(a.y, b.y), fmodf(a.z, b.z)); }
static inline Vec3 pow(const Vec3 &v1, const Vec3 &v2) { return Vec3(powf(v1.x, v2.x), powf(v1.y, v2.y), powf(v1.z, v2.z)); }

//------------------------------------------------------------------------------
// Vec3d
//------------------------------------------------------------------------------

struct Vec3d {
	union {
		struct {
			float x, y, z;
		};
		float data[3];
	};

	Vec3d() = default;
	Vec3d(float ax, float ay, float az): x(ax), y(ay), z(az) {}
	explicit Vec3d(float v): x(v), y(v), z(v) {}

	Vec3d &operator+=(const Vec3d &r) { x+=r.x; y+=r.y; z+=r.z; return *this; }
	Vec3d &operator-=(const Vec3d &r) { x-=r.x; y-=r.y; z-=r.z; return *this; }
	Vec3d &operator*=(const Vec3d &r) { x*=r.x; y*=r.y; z*=r.z; return *this; }
	Vec3d &operator/=(const Vec3d &r) { x/=r.x; y/=r.y; z/=r.z; return *this; }

	float &operator[](int i) { return data[i]; }
	float operator[](int i) const { return data[i]; }

	Vec2 XY() const { return {x, y}; }
	Vec2 XZ() const { return {x, z}; }
	Vec2 YZ() const { return {y, z}; }
};

// Helper constructors
static inline Vec3d Vec3d_X(float v = 1.0f) { return {v, 0, 0}; }
static inline Vec3d Vec3d_Y(float v = 1.0f) { return {0, v, 0}; }
static inline Vec3d Vec3d_Z(float v = 1.0f) { return {0, 0, v}; }

// Comparison operators
static inline bool operator==(const Vec3d &l, const Vec3d &r) { return l.x == r.x && l.y == r.y && l.z == r.z; }
static inline bool operator!=(const Vec3d &l, const Vec3d &r) { return l.x != r.x || l.y != r.y || l.z != r.z; }
static inline bool operator<(const Vec3d &l, const Vec3d &r)  { return l.x < r.x && l.y < r.y && l.z < r.z; }
static inline bool operator>(const Vec3d &l, const Vec3d &r)  { return l.x > r.x && l.y > r.y && l.z > r.z; }
static inline bool operator<=(const Vec3d &l, const Vec3d &r) { return l.x <= r.x && l.y <= r.y && l.z <= r.z; }
static inline bool operator>=(const Vec3d &l, const Vec3d &r) { return l.x >= r.x && l.y >= r.y && l.z >= r.z; }

// Unary operators
static inline Vec3d operator-(const Vec3d &v) { return {-v.x, -v.y, -v.z}; }

// Binary operators (Vec3d vs Vec3d)
static inline Vec3d operator+(const Vec3d &l, const Vec3d &r) { return {l.x + r.x, l.y + r.y, l.z + r.z}; }
static inline Vec3d operator-(const Vec3d &l, const Vec3d &r) { return {l.x - r.x, l.y - r.y, l.z - r.z}; }
static inline Vec3d operator*(const Vec3d &l, const Vec3d &r) { return {l.x * r.x, l.y * r.y, l.z * r.z}; }
static inline Vec3d operator/(const Vec3d &l, const Vec3d &r) { return {l.x / r.x, l.y / r.y, l.z / r.z}; }

// Functions
static inline bool is_nan(const Vec3d &v) { return std::isnan(v.x) || std::isnan(v.y) || std::isnan(v.z); }
static inline float length2(const Vec3d &v) { return v.x*v.x + v.y*v.y + v.z*v.z; }
static inline float length(const Vec3d &v) { return std::sqrt(length2(v)); }
static inline Vec3d normalize(const Vec3d &v) { return v / Vec3d(length(v)); }
static inline Vec3d abs(const Vec3d &v) { return {std::abs(v.x), std::abs(v.y), std::abs(v.z)}; }
static inline float dot(const Vec3d &v1, const Vec3d &v2) { return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z; }
static inline float volume(const Vec3d &v) { return v.x * v.y * v.z; }
static inline Vec3d cross(const Vec3d &v1, const Vec3d &v2) { return {v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x}; }
static inline float distance2(const Vec3d &v1, const Vec3d &v2) { return length2(v2-v1); }
static inline float distance(const Vec3d &v1, const Vec3d &v2) { return length(v2-v1); }
static inline Vec3d min(const Vec3d &v1, const Vec3d &v2) { return {min(v1.x, v2.x), min(v1.y, v2.y), min(v1.z, v2.z)}; }
static inline Vec3d max(const Vec3d &v1, const Vec3d &v2) { return {max(v1.x, v2.x), max(v1.y, v2.y), max(v1.z, v2.z)}; }
static inline Vec3d lerp(const Vec3d &a, const Vec3d &b, float v) { return a * Vec3d(1 - v) + b * Vec3d(v); }
static inline Vec3d mod(const Vec3d &a, const Vec3d &b) { return Vec3d(std::fmod(a.x, b.x), std::fmod(a.y, b.y), std::fmod(a.z, b.z)); }
static inline Vec3d pow(const Vec3d &v1, const Vec3d &v2) { return Vec3d(std::pow(v1.x, v2.x), std::pow(v1.y, v2.y), std::pow(v1.z, v2.z)); }

//------------------------------------------------------------------------------
// Vec3i
//------------------------------------------------------------------------------

struct Vec3i {
	union {
		struct {
			int x, y, z;
		};
		int data[3];
	};

	Vec3i() = default;
	Vec3i(int ax, int ay, int az): x(ax), y(ay), z(az) {}
	explicit Vec3i(int v): x(v), y(v), z(v) {}

	Vec3i &operator+=(const Vec3i &r) { x+=r.x; y+=r.y; z+=r.z; return *this; }
	Vec3i &operator-=(const Vec3i &r) { x-=r.x; y-=r.y; z-=r.z; return *this; }
	Vec3i &operator*=(const Vec3i &r) { x*=r.x; y*=r.y; z*=r.z; return *this; }
	Vec3i &operator/=(const Vec3i &r) { x/=r.x; y/=r.y; z/=r.z; return *this; }
	Vec3i &operator&=(const Vec3i &r) { x&=r.x; y&=r.y; z&=r.z; return *this; }

	int &operator[](int i) { return data[i]; }
	int operator[](int i) const { return data[i]; }

	Vec2i XY() const { return {x, y}; }
	Vec2i XZ() const { return {x, z}; }
	Vec2i YZ() const { return {y, z}; }
};

// Helper constructors
static inline Vec3i Vec3i_X(int v = 1) { return {v, 0, 0}; }
static inline Vec3i Vec3i_Y(int v = 1) { return {0, v, 0}; }
static inline Vec3i Vec3i_Z(int v = 1) { return {0, 0, v}; }
static inline Vec3i Vec3i_XY(const Vec2i &v) { return {v.x, v.y, 0}; }
static inline Vec3i Vec3i_XZ(const Vec2i &v) { return {v.x, 0, v.y}; }
static inline Vec3i Vec3i_YZ(const Vec2i &v) { return {0, v.x, v.y}; }

// Comparison operators
static inline bool operator==(const Vec3i &l, const Vec3i &r) { return l.x == r.x && l.y == r.y && l.z == r.z; }
static inline bool operator!=(const Vec3i &l, const Vec3i &r) { return l.x != r.x || l.y != r.y || l.z != r.z; }
static inline bool operator<(const Vec3i &l, const Vec3i &r)  { return l.x < r.x && l.y < r.y && l.z < r.z; }
static inline bool operator>(const Vec3i &l, const Vec3i &r)  { return l.x > r.x && l.y > r.y && l.z > r.z; }
static inline bool operator<=(const Vec3i &l, const Vec3i &r) { return l.x <= r.x && l.y <= r.y && l.z <= r.z; }
static inline bool operator>=(const Vec3i &l, const Vec3i &r) { return l.x >= r.x && l.y >= r.y && l.z >= r.z; }

// Unary operators
static inline Vec3i operator-(const Vec3i &v) { return {-v.x, -v.y, -v.z}; }
static inline Vec3i operator~(const Vec3i &v) { return {~v.x, ~v.y, ~v.z}; }

// Binary operators (Vec3i vs Vec3i)
static inline Vec3i operator+(const Vec3i &l, const Vec3i &r) { return {l.x + r.x, l.y + r.y, l.z + r.z}; }
static inline Vec3i operator-(const Vec3i &l, const Vec3i &r) { return {l.x - r.x, l.y - r.y, l.z - r.z}; }
static inline Vec3i operator*(const Vec3i &l, const Vec3i &r) { return {l.x * r.x, l.y * r.y, l.z * r.z}; }
static inline Vec3i operator/(const Vec3i &l, const Vec3i &r) { return {l.x / r.x, l.y / r.y, l.z / r.z}; }
static inline Vec3i operator^(const Vec3i &l, const Vec3i &r) { return {l.x ^ r.x, l.y ^ r.y, l.z ^ r.z}; }
static inline Vec3i operator%(const Vec3i &l, const Vec3i &r) { return {l.x % r.x, l.y % r.y, l.z % r.z}; }
static inline Vec3i operator&(const Vec3i &l, const Vec3i &r) { return {l.x & r.x, l.y & r.y, l.z & r.z}; }
static inline Vec3i operator|(const Vec3i &l, const Vec3i &r) { return {l.x | r.x, l.y | r.y, l.z | r.z}; }

// Functions
static inline int length2(const Vec3i &v) { return v.x*v.x + v.y*v.y + v.z*v.z; }
static inline Vec3i abs(const Vec3i &v) { return {abs(v.x), abs(v.y), abs(v.z)}; }
static inline int dot(const Vec3i &v1, const Vec3i &v2) { return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z; }
static inline int volume(const Vec3i &v) { return v.x * v.y * v.z; }
static inline Vec3i cross(const Vec3i &v1, const Vec3i &v2) { return {v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x}; }
static inline int distance2(const Vec3i &v1, const Vec3i &v2) { return length2(v2-v1); }
static inline Vec3i min(const Vec3i &v1, const Vec3i &v2) { return {min(v1.x, v2.x), min(v1.y, v2.y), min(v1.z, v2.z)}; }
static inline Vec3i max(const Vec3i &v1, const Vec3i &v2) { return {max(v1.x, v2.x), max(v1.y, v2.y), max(v1.z, v2.z)}; }
static inline Vec3i floor(const Vec3 &v) { return Vec3i(floorf(v.x), floorf(v.y), floorf(v.z)); }
static inline Vec3i floor(const Vec3d &v) { return Vec3i(std::floor(v.x), std::floor(v.y), std::floor(v.z)); }
static inline Vec3i floor_div(const Vec3i &a, const Vec3i &b) { return {floor_div(a.x, b.x), floor_div(a.y, b.y), floor_div(a.z, b.z)}; }

static inline Vec3 ToVec3(const Vec3i &v) { return Vec3(v.x, v.y, v.z); }
static inline Vec3 ToVec3(const Vec3d &v) { return Vec3(v.x, v.y, v.z); }
static inline Vec3d ToVec3d(const Vec3i &v) { return Vec3d(v.x, v.y, v.z); }
static inline Vec3d ToVec3d(const Vec3 &v) { return Vec3d(v.x, v.y, v.z); }
static inline Vec3i ToVec3i(const Vec3 &v) { return Vec3i(v.x, v.y, v.z); }
static inline Vec3i ToVec3i(const Vec3d &v) { return Vec3i(v.x, v.y, v.z); }

static inline bool axes_equal(const Vec3i &a, const Vec3i &b, const Vec2i &axes)
{
	return a[axes[0]] == b[axes[0]] && a[axes[1]] == b[axes[1]];
}

static inline bool aabb_aabb_intersection(const Vec3i &amin, const Vec3i &amax,
	const Vec3i &bmin, const Vec3i &bmax)
{
	return !(
		amax.x < bmin.x ||
		amax.y < bmin.y ||
		amax.z < bmin.z ||
		amin.x > bmax.x ||
		amin.y > bmax.y ||
		amin.z > bmax.z
	);
}

//------------------------------------------------------------------------------
// Vec4
//------------------------------------------------------------------------------

struct Vec4 {
	union {
		struct {
			float x, y, z, w;
		};
		float data[4];
	};

	Vec4() = default;
	Vec4(float x, float y, float z, float w): x(x), y(y), z(z), w(w) {}
	explicit Vec4(float v): x(v), y(v), z(v), w(v) {}

	Vec4 &operator+=(const Vec4 &r) { x+=r.x; y+=r.y; z+=r.z; w+=r.w; return *this; }
	Vec4 &operator-=(const Vec4 &r) { x-=r.x; y-=r.y; z-=r.z; w-=r.w; return *this; }
	Vec4 &operator*=(const Vec4 &r) { x*=r.x; y*=r.y; z*=r.z; w*=r.w; return *this; }
	Vec4 &operator/=(const Vec4 &r) { x/=r.x; y/=r.y; z/=r.z; w/=r.w; return *this; }

	float &operator[](int i) { return data[i]; }
	float operator[](int i) const { return data[i]; }
};

static inline Vec4 operator+(const Vec4 &l, const Vec4 &r) { return {l.x + r.x, l.y + r.y, l.z + r.z, l.w + r.w}; }
static inline Vec4 operator-(const Vec4 &l, const Vec4 &r) { return {l.x - r.x, l.y - r.y, l.z - r.z, l.w - r.w}; }
static inline Vec4 operator*(const Vec4 &l, const Vec4 &r) { return {l.x * r.x, l.y * r.y, l.z * r.z, l.w * r.w}; }
static inline Vec4 operator/(const Vec4 &l, const Vec4 &r) { return {l.x / r.x, l.y / r.y, l.z / r.z, l.w / r.w}; }

static inline float dot(const Vec4 &v1, const Vec4 &v2) { return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z + v1.w*v2.w; }

static inline Vec3 ToVec3(const Vec4 &v) { return Vec3(v.x, v.y, v.z); }
static inline Vec4 ToVec4(const Vec3 &v) { return Vec4(v.x, v.y, v.z, 1); }

//------------------------------------------------------------------------------
// Vec4i
//------------------------------------------------------------------------------

struct Vec4i {
	union {
		struct {
			int x, y, z, w;
		};
		int data[4];
	};

	Vec4i() = default;
	Vec4i(int x, int y, int z, int w): x(x), y(y), z(z), w(w) {}
	explicit Vec4i(int v): x(v), y(v), z(v), w(v) {}

	Vec4i &operator+=(const Vec4i &r) { x+=r.x; y+=r.y; z+=r.z; w+=r.w; return *this; }
	Vec4i &operator-=(const Vec4i &r) { x-=r.x; y-=r.y; z-=r.z; w-=r.w; return *this; }
	Vec4i &operator*=(const Vec4i &r) { x*=r.x; y*=r.y; z*=r.z; w*=r.w; return *this; }
	Vec4i &operator/=(const Vec4i &r) { x/=r.x; y/=r.y; z/=r.z; w/=r.w; return *this; }

	int &operator[](int i) { return data[i]; }
	int operator[](int i) const { return data[i]; }
};

static inline Vec4i operator+(const Vec4i &l, const Vec4i &r) { return {l.x + r.x, l.y + r.y, l.z + r.z, l.w + r.w}; }
static inline Vec4i operator-(const Vec4i &l, const Vec4i &r) { return {l.x - r.x, l.y - r.y, l.z - r.z, l.w - r.w}; }
static inline Vec4i operator*(const Vec4i &l, const Vec4i &r) { return {l.x * r.x, l.y * r.y, l.z * r.z, l.w * r.w}; }
static inline Vec4i operator/(const Vec4i &l, const Vec4i &r) { return {l.x / r.x, l.y / r.y, l.z / r.z, l.w / r.w}; }

static inline Vec4 ToVec4(const Vec4i &v) { return Vec4(v.x, v.y, v.z, v.w); }
static inline Vec4i ToVec4i(const Vec4 &v) { return Vec4i(v.x, v.y, v.z, v.w); }

//------------------------------------------------------------------------------
// Macro Utils
//------------------------------------------------------------------------------

#define VEC2(v) (v).x, (v).y
#define VEC3(v) (v).x, (v).y, (v).z
#define VEC4(v) (v).x, (v).y, (v).z, (v).w
